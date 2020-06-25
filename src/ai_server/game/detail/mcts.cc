#include <boost/math/constants/constants.hpp>
#include <boost/random.hpp>

#include "ai_server/util/math/angle.h"
#include "ai_server/util/math/to_vector.h"

#include "mcts.h"

namespace ai_server::game::detail::mcts {

// 行動
struct behavior {
  Eigen::Vector2d target_pos;
  behavior(const Eigen::Vector2d& p = Eigen::Vector2d::Zero()) : target_pos(p) {}
};

// 各スレッドで行うノード評価の処理および依存するデータを持つクラス
class worker {
private:
  boost::random::mt19937 mt_; // スレッド毎に持つ
  std::shared_ptr<nbla::utils::nnp::Executor> executor_;

  const model::field& field_;
  const model::world::robots_list& our_robots_;
  const model::world::robots_list& ene_robots_;
  const Eigen::Vector2d& our_goal_pos_;
  const Eigen::Vector2d& ene_goal_pos_;

  double evaluate(node& node);

  double playout(const state& state, double p);

  double probability(const Eigen::Vector2d& pos, const Eigen::Vector2d& target,
                     const model::world::robots_list& ene_robots);

  node& next_child_node(std::forward_list<node>::iterator begin,
                        std::forward_list<node>::iterator end);
  double score(const state& state);

  std::vector<behavior> legal_behaviors(const state& state);
  state next_state(const state& current_state, const behavior& b);
  bool end(const state& state);

public:
  worker(nbla::utils::nnp::Nnp& nnp, const model::field& field,
         const model::world::robots_list& our_robots,
         const model::world::robots_list& ene_robots, const Eigen::Vector2d& our_goal_pos,
         const Eigen::Vector2d& ene_goal_pos);

  void run(node& root_node, const std::chrono::steady_clock::duration& duration);

  void expand(node& node);
};

evaluator::evaluator(const game::nnabla& nnabla) {
  const auto num_threads = std::max(std::thread::hardware_concurrency() - 1, 1);
  nnp_ptrs_.reserve(num_threads);
  const bool probability_cached = num_threads < 2;
  const auto [ctx, path] =
      nnabla.nnp(probability_key_, probability_data_type_, probability_cached);
  for (int i = 0; i < num_threads; ++i) {
    auto nnp_ptr = std::make_unique<nbla::utils::nnp::Nnp>(ctx);
    nnp_ptr->add(path);
    nnp_ptrs_.push_back(std::move(nnp_ptr));
  }
}

void evaluator::execute(const model::field& field, const model::world::robots_list& our_robots,
                        const model::world::robots_list& ene_robots,
                        const Eigen::Vector2d& our_goal_pos,
                        const Eigen::Vector2d& ene_goal_pos, node& root_node) {
  using namespace std::chrono_literals;
  // スレッド数分の worker オブジェクトを作る
  std::vector<worker> workers;
  workers.reserve(nnp_ptrs_.size());
  for (auto& nnp_ptr : nnp_ptrs_)
    workers.emplace_back(*nnp_ptr, field, our_robots, ene_robots, our_goal_pos, ene_goal_pos);

  // 初回の expand
  workers.begin()->expand(root_node);

  // 各スレッドで worker を run()
  std::vector<std::thread> threads;
  threads.reserve(workers.size());
  for (auto& w : workers) threads.emplace_back(&worker::run, &w, std::ref(root_node), 10ms);
  for (auto& th : threads) th.join();
}

worker::worker(nbla::utils::nnp::Nnp& nnp, const model::field& field,
               const model::world::robots_list& our_robots,
               const model::world::robots_list& ene_robots, const Eigen::Vector2d& our_goal_pos,
               const Eigen::Vector2d& ene_goal_pos)
    : mt_(std::random_device{}()),
      executor_(nnp.get_executor("Executor")),
      field_(field),
      our_robots_(our_robots),
      ene_robots_(ene_robots),
      our_goal_pos_(our_goal_pos),
      ene_goal_pos_(ene_goal_pos) {
  executor_->set_batch_size(1);
}

void worker::run(node& root_node, const std::chrono::steady_clock::duration& duration) {
  const auto mcts_start = std::chrono::steady_clock::now();
  while ((std::chrono::steady_clock::now() - mcts_start) < duration) evaluate(root_node);
}

double worker::evaluate(node& node) {
  // 子ノードの範囲を取得
  std::unique_lock lock(node.mtx); // 別スレッドが子ノードを追加すると困るのでまずはロック
  node.n++;
  const auto state       = node.state;
  const double p         = node.p;
  const int n            = node.n;
  auto child_nodes_begin = node.child_nodes.begin();
  auto child_nodes_end   = node.child_nodes.end();

  if (end(state)) {
    // 終了状態なら期待報酬計算
    const double v = node.p * score(state);
    node.w += v;
    return v;
  }

  const bool child_nodes_empty = child_nodes_begin == child_nodes_end;
  // 訪問数が一定数に到達していれば木を拡張
  if (child_nodes_empty && n == 10) {
    lock.unlock();
    expand(node);
  }

  if (child_nodes_empty) {
    // 葉ノードならプレイアウト
    if (lock) lock.unlock();
    const double v = playout(state, p);
    lock.lock();
    node.w += v;
    return v;
  } else {
    // 再帰で葉ノードからの値を待つ
    if (lock) lock.unlock();
    auto& next_node = next_child_node(child_nodes_begin, child_nodes_end);
    const auto v    = evaluate(next_node);
    lock.lock();
    node.w += v;
    return v;
  }
}

void worker::expand(node& node) {
  double max_score = 0.0;
  std::vector<std::pair<behavior, double>> behavior_probability;
  std::unique_lock lock(node.mtx);
  const auto state = node.state;
  lock.unlock();
  for (const auto& b : legal_behaviors(state)) {
    const double p = probability(state.ball_pos, b.target_pos, ene_robots_);
    const double s = p * score(next_state(state, b));
    if (max_score < s) max_score = s;
    behavior_probability.emplace_back(b, p);
  }
  lock.lock();
  for (const auto& bp : behavior_probability) {
    if (bp.second < max_score) continue;
    node.child_nodes.emplace_front(next_state(state, bp.first), node.p * bp.second);
  }
}

double worker::playout(const state& state, double p) {
  if (end(state)) return p * score(state);

  const double p_to_goal = probability(state.ball_pos, ene_goal_pos_, ene_robots_);
  if (p_to_goal > 0.8) {
    return playout(next_state(state, behavior{ene_goal_pos_}), p * p_to_goal);
  } else {
    const auto behaviors = legal_behaviors(state);
    std::uniform_int_distribution<std::size_t> dist(0, static_cast<int>(behaviors.size()) - 1);
    const behavior selected_b = behaviors.at(dist(mt_));
    return playout(next_state(state, selected_b),
                   p * probability(state.ball_pos, selected_b.target_pos, ene_robots_));
  }
}

double worker::probability(const Eigen::Vector2d& pos, const Eigen::Vector2d& target,
                           const model::world::robots_list& ene_robots) {
  using boost::math::constants::pi;
  constexpr double ball_speed  = 6000.0;
  constexpr double robot_speed = 3000.0;
  if ((pos - target).norm() > 10000.0) return 0;
  // 入出力データは必ずCPUのコンテキストで扱う
  const nbla::Context cpu_ctx{{"cpu:float"}, "CpuArray", "0"};
  nbla::CgVariablePtr in_ptr = executor_->get_data_variables().at(0).variable;
  float* in_data             = in_ptr->variable()->cast_data_and_get_pointer<float>(cpu_ctx);
  in_data[0]                 = static_cast<float>(ball_speed / 10000.0);

  double probability = 0.99;
  for (const auto& r : ene_robots) {
    const Eigen::Vector2d ene_pos = util::math::position(r.second);
    const double theta =
        util::math::wrap_to_pi(std::atan2(ene_pos.y() - pos.y(), ene_pos.x() - pos.x()) -
                               std::atan2(target.y() - pos.y(), target.x() - pos.x()));
    // チップ
    if ((pos - target).norm() < 2500.0 && (ene_pos - pos).norm() < 1000.0 &&
        std::abs(theta) < 1.0)
      continue;
    const double sin_dist = std::abs((ene_pos - pos).norm() * std::sin(theta));
    const double cos_dist = std::abs((ene_pos - pos).norm() * std::cos(theta));
    if (std::abs(theta) < 0.5 * pi<double>() &&
        sin_dist < robot_speed * cos_dist / ball_speed && cos_dist < (target - pos).norm()) {
      in_data[1] = static_cast<float>((ene_pos - pos).norm() / 10000.0);
      in_data[2] = static_cast<float>(std::abs(theta) / pi<double>());
      // execute
      executor_->execute();
      nbla::CgVariablePtr out_ptr = executor_->get_output_variables().at(0).variable;
      const float* out_data       = out_ptr->variable()->get_data_pointer<float>(cpu_ctx);
      const double tmp_p          = static_cast<double>(out_data[0]);

      constexpr bool use_all_enemy = true;
      if (use_all_enemy) {
        // 対象領域内の全ロボットからの危険度を考慮する
        probability *= tmp_p;
      } else {
        // 一番危険なロボットからの危険度のみ考慮する
        if (tmp_p < probability) probability = tmp_p;
      }
    }
  }
  return probability;
}

node& worker::next_child_node(std::forward_list<node>::iterator begin,
                              std::forward_list<node>::iterator end) {
  std::vector<std::tuple<std::forward_list<node>::iterator, int, double>> v{};
  double t = 0.0;
  for (auto i = begin; i != end; ++i) {
    // 子ノードの値を読む前に lock
    std::unique_lock lock(i->mtx);
    if (i->n == 0) return *i;
    t += i->n;
    // あとで必要となる値を読み込んでおく
    v.emplace_back(i, i->n, i->w);
  }
  // 2ループ目は v を使って子ノードの lock を回避
  // その間に他スレッドによって行われるノードの変更は無視する
  auto e = std::max_element(
      v.begin(), v.end(), [two_log_t = 2.0 * std::log(t)](const auto& a, const auto& b) {
        return std::get<2>(a) / std::get<1>(a) + std::sqrt(two_log_t / std::get<1>(a)) <
               std::get<2>(b) / std::get<1>(b) + std::sqrt(two_log_t / std::get<1>(b));
      });
  return *std::get<0>(*e);
}

double worker::score(const state& state) {
  if ((state.ball_pos - ene_goal_pos_).norm() < 500.0)
    return 1.0;
  else if ((state.ball_pos - our_goal_pos_).norm() < 500.0)
    return -1.0;
  else
    return 0.0;
}

std::vector<behavior> worker::legal_behaviors(const state& state) {
  const double x_max          = field_.x_max();
  const double y_max          = field_.y_max();
  const double penalty_length = field_.penalty_length();
  const double penalty_width  = field_.penalty_width();
  std::vector<behavior> behaviors;
  behaviors.reserve(3 + our_robots_.size());
  behaviors.emplace_back(ene_goal_pos_);
  behaviors.emplace_back(Eigen::Vector2d(ene_goal_pos_.x(), 200.0));
  behaviors.emplace_back(Eigen::Vector2d(ene_goal_pos_.x(), -200.0));
  for (const auto& r : our_robots_) {
    const Eigen::Vector2d robot_pos = util::math::position(r.second);
    if ((robot_pos - state.ball_pos).norm() > 500.0 && std::abs(robot_pos.x()) < x_max &&
        std::abs(robot_pos.y()) < y_max &&
        (std::abs(robot_pos.x()) < x_max - penalty_length - 200.0 ||
         std::abs(robot_pos.y()) > penalty_width / 2.0 + 200.0))
      behaviors.emplace_back(robot_pos);
  }
  return behaviors;
}

state worker::next_state(const state& current_state, const behavior& b) {
  constexpr double ball_speed = 6000.0;
  const double t =
      current_state.t + (current_state.ball_pos - b.target_pos).norm() / ball_speed;
  return {b.target_pos, t};
}

bool worker::end(const state& state) {
  return (state.ball_pos - our_goal_pos_).norm() < 500.0 ||
         (state.ball_pos - ene_goal_pos_).norm() < 500.0;
}

} // namespace ai_server::game::detail::mcts