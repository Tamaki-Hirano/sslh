#include <cmath>
#include <map>

#include "ai_server/game/agent/defense.h"
#include "ai_server/model/command.h"
#include "ai_server/util/math/geometry.h"
#include "ai_server/util/math/to_vector.h"
#include "ai_server/util/math.h"

namespace ai_server {
namespace game {
namespace agent {

defense::defense(const model::world& world, bool is_yellow, unsigned int keeper_id,
                 const std::vector<unsigned int>& wall_ids,
                 const std::vector<unsigned int>& marking_ids)
    : base(world, is_yellow),
      keeper_id_(keeper_id),
      wall_ids_(wall_ids),
      marking_ids_(marking_ids),
      orientation_(Eigen::Vector2d::Zero()),
      mode_(defense_mode::normal_mode) {
  // actionの生成部分
  //
  // actionの初期化は一回しか行わない
  //
  //

  //キーパー用のaction
  keeper_     = std::make_shared<action::guard>(world_, is_yellow_, keeper_id_);
  keeper_get_ = std::make_shared<action::get_ball>(world_, is_yellow_, keeper_id_);

  //壁用のaction
  for (auto it : wall_ids_) {
    wall_.emplace_back(std::make_shared<action::guard>(world_, is_yellow_, it));
  }

  //クリア用のaction
  for (auto it : wall_ids_) {
    wall_get_.emplace_back(std::make_shared<action::get_ball>(world_, is_yellow_, it));
  }

  //マーキング用のaction
  for (auto it : marking_ids_) {
    marking_.emplace_back(std::make_shared<action::marking>(world_, is_yellow_, it));
  }
  ball_ = {0.0, 0.0};
}

defense::defense(const model::world& world, bool is_yellow, unsigned int keeper_id,
                 const std::vector<unsigned int>& wall_ids)
    : base(world, is_yellow),
      keeper_id_(keeper_id),
      wall_ids_(wall_ids),
      orientation_(Eigen::Vector2d::Zero()),
      mode_(defense_mode::normal_mode) {
  // actionの生成部分
  //
  // actionの初期化は一回しか行わない
  //
  //

  //キーパー用のaction
  keeper_     = std::make_shared<action::guard>(world_, is_yellow_, keeper_id_);
  keeper_get_ = std::make_shared<action::get_ball>(world_, is_yellow_, keeper_id_);

  //壁用のaction
  for (auto it : wall_ids_) {
    wall_.emplace_back(std::make_shared<action::guard>(world_, is_yellow_, it));
  }

  //クリア用のaction
  for (auto it : wall_ids_) {
    wall_get_.emplace_back(std::make_shared<action::get_ball>(world_, is_yellow_, it));
  }

  ball_ = {0.0, 0.0};
}

void defense::set_mode(defense_mode mode) {
  mode_ = mode;
}

std::vector<unsigned int> defense::marking_ids() const {
  return marking_ids_re_;
}

Eigen::Vector2d defense::calc_base_point(Eigen::Vector2d goal, Eigen::Vector2d ball,
                                         double radius) {
  //半径

  const auto length = (goal - ball).norm(); //ボール<->ゴール

  const auto ratio = radius / length; //全体に対しての基準座標の比

  Eigen::Vector2d tmp = (1 - ratio) * goal + ratio * ball;
  if (std::abs(tmp.y()) < 1200.0) {
    tmp.x() = world_.field().x_min() + 1360.0;
  } else {
    const auto A = (goal.y() - ball.y()) / (goal.x() - ball.x());
    const auto B = ball.y() - A * ball.x();
    const auto a = -1 * A;
    const auto b = 1;
    const auto c = -1 * B;
    Eigen::Vector2d pos_p{goal.x(), std::copysign(0, ball.y())};
    const auto d = a * pos_p.x() + b * pos_p.y() + c;
    Eigen::Vector2d pos_h{pos_p.x() - a * (d / (std::pow(a, 2) + std::pow(b, 2))),
                          pos_p.y() - b * (d / (std::pow(a, 2) + std::pow(b, 2)))};
    const auto r  = 1920.0;
    const auto hq = std::sqrt(std::pow(r, 2) - (d / (std::pow(a, 2) + std::pow(b, 2))));
    tmp           = Eigen::Vector2d{pos_h.x() + hq * (b / std::hypot(a, b)),
                          pos_h.y() - hq * (a / std::hypot(a, b))};
  }
  return tmp;
}

std::vector<std::shared_ptr<action::base>> defense::execute() {
  using boost::math::constants::pi;

  //ボールの座標
  const Eigen::Vector2d ball_vel(world_.ball().vx(), world_.ball().vy());
  // visionから取得した今のボールの位置
  const Eigen::Vector2d ball_pos(world_.ball().x(), world_.ball().y());
  const Eigen::Vector2d ball_k(ball_vel * 0.5);
  // 0.5s後のボールの位置(推定)
  const Eigen::Vector2d ball(ball_pos + ball_k);

  //状態を遷移させるためのボールの位置
  if (ball_vel.norm() < 150.0) {
    ball_ = ball_pos;
  }

  //ボールがゴールより後ろに来たら現状維持
  if (ball_pos.x() < world_.field().x_min() || ball_pos.x() > world_.field().x_max()) {
    for (auto wall_it : wall_) {
      wall_it->set_halt(true);
    }
    keeper_->set_halt(true);

    std::vector<std::shared_ptr<action::base>> re_wall{
        wall_.begin(), wall_.end()}; //型を合わせるために無理矢理作り直す
    re_wall.push_back(keeper_);      //配列を返すためにキーパーを統合する

    return re_wall;
  }
  for (auto wall_it : wall_) {
    wall_it->set_halt(false);
  }
  keeper_->set_halt(false);
  //ゴールの座標
  const Eigen::Vector2d goal(world_.field().x_min(), 0.0);

  //ボールとゴールの角度
  const auto ball_theta = std::atan2(ball.y() - goal.y(), ball.x() - goal.x());

  //基準座標を求めている
  orientation_ = calc_base_point(goal, ball, 1850.0);

  const auto ally_robots  = is_yellow_ ? world_.robots_yellow() : world_.robots_blue();
  const auto enemy_robots = is_yellow_ ? world_.robots_blue() : world_.robots_yellow();

  //ここから壁の処理
  //
  //
  //
  //
  //
  {
    //壁のイテレータ

    std::vector<Eigen::Vector2d> target;
    //移動目標を出す
    {
      //基準点からどれだけずらすか
      auto shift_ = 190.0;
      //縄張りの大きさ
      const auto demarcation = 3500.0;
      //壁の数によってずらしていく倍率が変わるのでその倍率
      auto magnification = 2.0;

      //敵がめっちゃ近づいたら閉める
      if ((ball - goal).norm() < demarcation) {
        shift_ = 90;
      }
      if (wall_.size() % 2 != 0) {
        target.push_back(orientation_);
        shift_        = 360.0;
        magnification = 1.0;
        if ((ball - goal).norm() < demarcation) {
          shift_ = 180.0;
        }
      }
      unsigned int i = 0;
      auto shift     = shift_;
      for (auto&& wall_it : wall_) {
        const auto tmp = util::math::calc_isosceles_vertexes(ball, orientation_, shift);
        target.push_back((i == 0) ? std::get<0>(tmp) : std::get<1>(tmp));
        shift += (i == 0) ? 0.0 : (shift_ * magnification);
        i = (i == 0) ? 1 : 0;
      }
      if (wall_.size() % 2 != 0) {
        target.pop_back();
      }
    }

    //実際にアクションを詰めて返す
    {
      //壁とゴールの角度順にソート
      std::sort(wall_.begin(), wall_.end(),
                [&goal, &ally_robots](const auto& a, const auto& b) {
                  return std::atan2(ally_robots.at(a->id()).y() - goal.y(),
                                    ally_robots.at(a->id()).x() - goal.x()) <
                         std::atan2(ally_robots.at(b->id()).y() - goal.y(),
                                    ally_robots.at(b->id()).x() - goal.x());
                });

      //目標地点とゴールの角度順にソート
      std::sort(target.begin(), target.end(), [&goal](const auto& a, const auto& b) {
        return std::atan2(a.y() - goal.y(), a.x() - goal.x()) <
               std::atan2(b.y() - goal.y(), b.x() - goal.x());
      });

      //割り当てる
      auto target_it = target.begin();
      for (auto wall_it : wall_) {
        if (ally_robots.find(wall_it->id()) != ally_robots.end()) {
          //壁がディフェンスエリアに入らないようにする
          if ((util::math::position(ally_robots.at(wall_it->id())) - goal).norm() < 1900.0) {
            if (std::abs(target_it->y()) > 1200.0 &&
                std::abs(util::math::position(ally_robots.at(wall_it->id())).y()) < 1200.0) {
              wall_it->move_to(goal.x() + 1360.0,
                               std::copysign(world_.field().width() / 4, target_it->y()),
                               ball_theta);
            } else if (std::abs(target_it->y()) < 1380.0 &&
                       std::abs(util::math::position(ally_robots.at(wall_it->id())).y()) >
                           1200.0) {
              wall_it->move_to(goal.x() + 1360.0, std::copysign(1360.0, target_it->y()),
                               ball_theta);
            } else {
              wall_it->move_to((*target_it).x(), (*target_it).y(), ball_theta);
            }
          } else {
            wall_it->move_to((*target_it).x(), (*target_it).y(), ball_theta);
          }
        }

        if (mode_ == defense_mode::stop_mode) {
          wall_it->set_kick_type({model::command::kick_type_t::none, 0});
        } else {
          wall_it->set_kick_type({model::command::kick_type_t::chip, 255});
        }

        wall_it->set_dribble(0);
        const auto tmp = ball_vel.norm();

        wall_it->set_magnification(tmp);
        target_it++;
      }
    }
  }
  //型を合わせるために無理矢理作り直す
  std::vector<std::shared_ptr<action::base>> re_wall{wall_.begin(), wall_.end()};

  //クリアさせる
  {
    if (!wall_ids_.empty() && mode_ != defense_mode::stop_mode) {
      if (!enemy_robots.empty()) {
        //ボールに一番近いロボットを求める
        const auto it = std::min_element(
            enemy_robots.cbegin(), enemy_robots.cend(), [&ball](auto&& a, auto&& b) {
              const auto l1 = Eigen::Vector2d{a.second.x(), a.second.y()} - ball;
              const auto l2 = Eigen::Vector2d{b.second.x(), b.second.y()} - ball;
              return l1.norm() < l2.norm();
            });
        const auto r = std::get<1>(*it);

        //ボール付近に敵がいないときだけクリアする
        if ((goal - ball_pos).norm() < 2000.0 && (goal - ball_pos).norm() > 1400 &&
            ball_vel.norm() < 200.0 && ball_pos.x() > -4000.0 &&
            (ball_pos - Eigen::Vector2d{r.x(), r.y()}).norm() > 300.0) {
          // ボールに最も近い味方ロボットを求める
          const auto it = std::min_element(
              re_wall.cbegin(), re_wall.cend(), [&ally_robots, &ball](auto&& a, auto&& b) {
                const auto l1 =
                    Eigen::Vector2d{ally_robots.at(a->id()).x(), ally_robots.at(a->id()).y()} -
                    ball;
                const auto l2 =
                    Eigen::Vector2d{ally_robots.at(b->id()).x(), ally_robots.at(b->id()).y()} -
                    ball;
                return l1.norm() < l2.norm();
              });
          const auto id = (*it)->id();
          re_wall.erase(it);
          //クリアように追加する
          for (auto it : wall_get_) {
            if (it->id() == id) {
              it->set_chip(true);
              re_wall.push_back(it);
            }
          }
        }
      }
    }
  }

  //マーキングの処理.

  {
    if (!marking_.empty()) {
      std::vector<enemy> enemy_list;
      if (!enemy_robots.empty()) {
        for (auto it : enemy_robots) {
          const Eigen::Vector2d tmp{(it.second).x(), (it.second).y()};
          if (((ball - tmp).norm() < 1000.0) || (tmp.x() > 3000.0)) {
            continue;
          }
          enemy_list.emplace_back(enemy{it.first, tmp, (it.second).theta(), 0.0, 0});
        }
      }
      //優先順位の評価部分
      {
        //座標で点数決め
        for (auto& it : enemy_list) {
          it.valuation = it.position.y();
        }
        //距離が近い順に昇順ソート
        std::sort(enemy_list.begin(), enemy_list.end(),
                  [&ball](const enemy& a, const enemy& b) {
                    return std::signbit(ball.y()) ? a.valuation > b.valuation
                                                  : a.valuation < b.valuation;
                  });
        //点数の初期化
        auto point = enemy_list.size();
        for (auto& it : enemy_list) {
          it.score = point--;
        }
      }

      //点数が大きい順に並べ替える
      std::sort(enemy_list.begin(), enemy_list.end(),
                [](const enemy& a, const enemy& b) { return (a.score > b.score); });

      //近い順に割り当てる
      {
        //マーキングに割り当てられたロボットのidとactionのペア
        std::unordered_map<unsigned int, mark> mark_list;
        const auto mark_robots = is_yellow_ ? world_.robots_yellow() : world_.robots_blue();
        for (const auto& it : marking_) {
          if (mark_robots.count(it->id())) {
            const mark robot{{mark_robots.at(it->id()).x(), mark_robots.at(it->id()).y()}, it};
            mark_list.insert(std::make_pair(it->id(), robot));
          }
        }

        //敵を起点として最近傍探索
        for (auto enemy_it = enemy_list.begin();
             !mark_list.empty() && enemy_it != enemy_list.end(); enemy_it++) {
          const auto it = std::min_element(mark_list.cbegin(), mark_list.cend(),
                                           [enemy_it](auto&& a, auto&& b) {
                                             const auto enemy_pos = enemy_it->position;
                                             const auto l1 = enemy_pos - a.second.position;
                                             const auto l2 = enemy_pos - b.second.position;
                                             return l1.norm() < l2.norm();
                                           });
          const auto& r = std::get<1>(*it);
          r.action->mark_robot(enemy_it->id);
          r.action->set_mode(action::marking::mark_mode::shoot_block);
          r.action->set_radius(250.0);
          if (enemy_it == enemy_list.begin()) {
            r.action->set_radius(600.0);
            r.action->set_mode(game::action::marking::mark_mode::corner_block);
          }
          mark_list.erase(it);
        }
        //もしマークロボットが溢れたらこうなる
        //味方の優先順位の低いやつを返す
        marking_ids_re_.clear();
        if (!mark_list.empty()) {
          for (const auto& it : mark_list) {
            marking_ids_re_.push_back(it.first);
          }
        }
      }
    }
  }
  re_wall.insert(re_wall.end(), marking_.begin(),
                 marking_.end()); //配列を返すためにマーキングをを統合する

  //ここからキーパーの処理
  //
  //
  //
  {
    Eigen::Vector2d keeper(Eigen::Vector2d::Zero());

    bool flag = false;
    switch (mode_) {
      case defense_mode::stop_mode: {
      }
      case defense_mode::normal_mode: {
        //キーパーはボールの位置によって動き方が2種類ある.
        // そもそもマーキング状態なら別の処理
        // C:ボールが縄張りに入ってきた！やばーい
        // B:ボールが自陣地なので壁の補強をしなければ
        const auto demarcation1 = 1000.0; //クリアする範囲
        const auto demarcation2 = 400.0;  //クリアする範囲
        const auto demarcation3 = 3500.0; //縄張りの大きさ

        if ((ball_ - goal).norm() < demarcation1 && (ball_ - goal).norm() > demarcation2 &&
            ball_vel.norm() < 500.0 && mode_ != defense_mode::stop_mode) {
          flag = true;
          keeper_get_->set_chip(true);
        } else if (((ball_ - goal).norm() < demarcation3)) { // C
          const auto A = (ball.y() - ball_pos.y()) / (ball.x() - ball_pos.x());
          const auto B = ball_pos.y() - A * ball_pos.x();
          //ゴール前でディフェンスする
          if (std::abs(A * goal.x() + B) < 500.0 && ball_vel.norm() > 500.0) {
            const auto a = -1 * A;
            const auto b = 1;
            const auto c = -1 * B;
            Eigen::Vector2d pos_p{goal};
            const auto d = a * pos_p.x() + b * pos_p.y() + c;
            Eigen::Vector2d pos_h{pos_p.x() - a * (d / (std::pow(a, 2) + std::pow(b, 2))),
                                  pos_p.y() - b * (d / (std::pow(a, 2) + std::pow(b, 2)))};
            const auto r  = 400.0;
            const auto hq = std::sqrt(std::pow(r, 2) - (d / (std::pow(a, 2) + std::pow(b, 2))));
            keeper        = Eigen::Vector2d{pos_h.x() + hq * (b / std::hypot(a, b)),
                                     pos_h.y() - hq * (a / std::hypot(a, b))};

          } else {
            const auto length = (goal - orientation_).norm(); //基準点<->ボール
            const auto ratio  = (500) / length; //全体に対してのキーパー位置の比
            keeper            = (1 - ratio) * goal + ratio * orientation_;
            keeper_->set_kick_type({model::command::kick_type_t::none, 0});
            keeper_->set_dribble(0);
          }
        } else { /*B*/
          const auto A = (ball.y() - ball_pos.y()) / (ball.x() - ball_pos.x());
          const auto B = ball_pos.y() - A * ball_pos.x();
          if (std::abs(A * goal.x() + B) < 500.0 && ball_vel.norm() > 500.0) {
            const auto a = -1 * A;
            const auto b = 1;
            const auto c = -1 * B;
            Eigen::Vector2d pos_p{goal};
            const auto d = a * pos_p.x() + b * pos_p.y() + c;
            Eigen::Vector2d pos_h{pos_p.x() - a * (d / (std::pow(a, 2) + std::pow(b, 2))),
                                  pos_p.y() - b * (d / (std::pow(a, 2) + std::pow(b, 2)))};
            const auto r  = 900.0;
            const auto hq = std::sqrt(std::pow(r, 2) - (d / (std::pow(a, 2) + std::pow(b, 2))));
            keeper        = Eigen::Vector2d{pos_h.x() + hq * (b / std::hypot(a, b)),
                                     pos_h.y() - hq * (a / std::hypot(a, b))};
          } else {
            keeper_->set_kick_type({model::command::kick_type_t::none, 0});
            keeper_->set_dribble(0);
            //壁のすぐ後ろで待機
            //基準点からちょっと下がったキーパの位置
            const auto length = (goal - ball).norm(); //基準点<->ボール
            const auto ratio  = (900) / length; //全体に対してのキーパー位置の比
            keeper            = (1 - ratio) * goal + ratio * ball;
          }
        }
        const auto tmp = (ball_vel.norm() * 1.0 < 1000.0) ? 1000.0 : ball_vel.norm() * 1.0;
        keeper_->set_magnification(tmp);
        break;
      }
      case defense_mode::pk_normal_mode: {
        if (!enemy_robots.empty()) {
          //敵のシューターを線形探索する
          //ボールにもっとも近いやつがシューターだと仮定
          // ボールに最も近い敵ロボットを求める
          const auto it = std::min_element(
              enemy_robots.cbegin(), enemy_robots.cend(), [&ball](auto&& a, auto&& b) {
                const auto l1 = Eigen::Vector2d{a.second.x(), a.second.y()} - ball;
                const auto l2 = Eigen::Vector2d{b.second.x(), b.second.y()} - ball;
                return l1.norm() < l2.norm();
              });
          const auto r = std::get<1>(*it);
          //キーパーのy座標は敵シューターの視線の先
          keeper.y() = (1000.0 * std::tan(r.theta())) * (-1);

          //ゴールの範囲を超えたら跳びでないようにする
          if (keeper.y() > 410) {
            keeper.y() = 410;
          } else if (keeper.y() < -410) {
            keeper.y() = -410;
          }

          //ロボットの大きさ分ずらす
          keeper.x() = goal.x() + 70.0;

          keeper_->set_magnification(3500.0);
          keeper_->set_dribble(0);
        }
        break;
      }
      case defense_mode::pk_extention_mode: {
        keeper = ball_pos;
        keeper.x() += 45.0;
        keeper_->set_magnification(6000.0);
        keeper_->set_dribble(9);
        break;
      }
    }
    keeper_->move_to(keeper.x(), keeper.y(), ball_theta);
    if (flag) {
      re_wall.push_back(keeper_get_); //配列を返すためにキーパーを統合する
    } else {
      re_wall.push_back(keeper_); //配列を返すためにキーパーを統合する
    }
  }

  return re_wall; //返す
}
} // namespace agent
} // namespace game
} // namespace ai_server
