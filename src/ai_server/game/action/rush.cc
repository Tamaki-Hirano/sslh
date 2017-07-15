#include "ai_server/game/action/rush.h"

#include <cmath>

namespace ai_server {
namespace game {
namespace action {

rush::rush(const model::world& world, bool is_yellow, unsigned int id)
    : base(world, is_yellow, id),
      kick_type_({model::command::kick_type_t::none, 0.0}),
      flag_(false),
      previous_kick_ball_(world_.ball()) {}

model::command rush::execute() {
  model::command command(id_);

  const auto robots = is_yellow_ ? world_.robots_yellow() : world_.robots_blue();
  const auto ball   = world_.ball();

  //見えなかったら止める
  if (!robots.count(id_)) {
    command.set_velocity({0.0, 0.0, 0.0});
    return command;
  }

  //速度計算
  const auto& robot = robots.at(id_);
  double theta      = std::atan2(ball.y() - robot.y(), ball.x() - robot.x());
  double omega      = theta - robot.theta();
  double vx         = 2000 * std::cos(theta);
  double vy         = 2000 * std::sin(theta);

  //ボールを蹴ったと判断したら終了
  if (std::hypot(previous_kick_ball_.x() - ball.x(), previous_kick_ball_.y() - ball.y()) >
      200) {
    command.set_velocity({0.0, 0.0, omega});
    flag_ = true;
    return command;
  }

  //速度を与えて動作させる
  command.set_kick_flag({model::command::kick_type_t::line, 10});
  command.set_velocity({vx, vy, omega});

  flag_               = false;
  previous_kick_ball_ = ball;
  return command;
}

bool rush::finished() const {
  return flag_;
}
}
}
}