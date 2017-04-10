#include <boost/math/constants/constants.hpp>
#include <cmath>

#include "ai_server/util/math.h"
#include "pid_controller.h"
#include <boost/algorithm/clamp.hpp>
#include <cmath>

namespace ai_server {
namespace controller {

const double pid_controller::kp_[]             = {2.0, 4.0};
const double pid_controller::ki_[]             = {0.2, 0.2};
const double pid_controller::kd_[]             = {0.0, 0.0};
const double pid_controller::max_velocity_     = 3000.0; // 最大速度
const double pid_controller::max_acceleration_ = 2000.0; // 最大加速度
const double pid_controller::min_acceleration_ = 100.0;  // 速度ゼロ時の加速度
// optimized_accelがmax_accelに到達するときのロボット速度
const double pid_controller::reach_speed_ = 1500.0;

// model::command::velocityの四則演算のオーバロード
const velocity_t operator+(const velocity_t& vel1, const velocity_t& vel2) {
  return {vel1.vx + vel2.vx, vel1.vy + vel2.vy, vel1.omega + vel2.omega};
}

const velocity_t operator-(const velocity_t& vel1, const velocity_t& vel2) {
  return {vel1.vx - vel2.vx, vel1.vy - vel2.vy, vel1.omega - vel2.omega};
}

const velocity_t operator*(const double& c, const velocity_t& vel) {
  return {c * vel.vx, c * vel.vy, c * vel.omega};
}

const velocity_t operator*(const velocity_t& vel1, const velocity_t& vel2) {
  return {vel1.vx * vel2.vx, vel1.vy * vel2.vy, vel1.omega * vel2.omega};
}

const velocity_t operator/(const velocity_t& vel, const double& c) {
  return {vel.vx / c, vel.vy / c, vel.omega / c};
}

pid_controller::pid_controller(double cycle) : cycle_(cycle) {
  for (int i = 0; i < 2; i++) {
    up_[i] = {0.0, 0.0, 0.0};
    ui_[i] = {0.0, 0.0, 0.0};
    ud_[i] = {0.0, 0.0, 0.0};
    u_[i]  = {0.0, 0.0, 0.0};
    e_[i]  = {0.0, 0.0, 0.0};
  }
}

velocity_t pid_controller::update(const model::robot& robot, const position_t& setpoint) {
  // 位置偏差
  position_t ep;
  ep.x     = setpoint.x - robot.x();
  ep.y     = setpoint.y - robot.y();
  ep.theta = util::wrap_to_pi(setpoint.theta - robot.theta());

  double speed     = std::hypot(ep.x, ep.y);
  double direction = util::wrap_to_pi(std::atan2(ep.y, ep.x) - robot.theta());
  e_[0].vx         = speed * std::cos(direction);
  e_[0].vy         = speed * std::sin(direction);
  e_[0].omega      = ep.theta;

  // 制御計算
  calculate();

  return u_[0];
}

velocity_t pid_controller::update(const model::robot& robot, const velocity_t& setpoint) {
  // 現在偏差
  e_[0].vx    = setpoint.vx - u_[1].vx;
  e_[0].vy    = setpoint.vy - u_[1].vy;
  e_[0].omega = setpoint.omega - u_[1].omega;

  // 制御計算
  calculate();

  return u_[0];
}

void pid_controller::calculate() {
  // 計算用にゲイン再計算
  velocity_t kp = model::command::velocity_t{kp_[0], kp_[0], kp_[1]};
  velocity_t ki = model::command::velocity_t{ki_[0], ki_[0], ki_[1]};
  velocity_t kd = model::command::velocity_t{kd_[0], kd_[0], kd_[1]};
  // 双一次変換
  // s=(2/T)*(Z-1)/(Z+1)としてPIDcontrollerを離散化
  // C=Kp+Ki/s+Kds
  up_[0] = kp * e_[0];
  ui_[0] = cycle_ * ki * (e_[0] + e_[1]) / 2 + ui_[1];
  ud_[0] = 2 * kd * (e_[0] - e_[1]) / cycle_ - ud_[1];
  u_[0]  = u_[1] + cycle_ * (up_[0] + ui_[0] + ud_[0]);

  // 加速度，速度制限
  using boost::algorithm::clamp;
  double u_angle         = std::atan2(u_[0].vy, u_[0].vx); // 今回指令速度の方向
  double u_speed         = std::hypot(u_[0].vx, u_[0].vy); // 今回指令速度の大きさ
  double preceding_speed = std::hypot(u_[1].vx, u_[1].vy); // 前回指令速度の大きさ
  double delta_speed = u_speed - preceding_speed; // 速さ偏差(今回速さと前回速さの差)
  // 速度に応じて加速度を変化(初動でのスリップ防止)
  // 制限加速度計算
  double optimized_accel =
      preceding_speed * (max_acceleration_ - min_acceleration_) / reach_speed_ +
      min_acceleration_;
  optimized_accel = clamp(optimized_accel, min_acceleration_, max_acceleration_);
  // 加速度制限
  if (delta_speed / cycle_ > optimized_accel &&
      std::abs(u_speed) > std::abs(preceding_speed)) { // +制限加速度超過
    u_speed = preceding_speed + (optimized_accel * cycle_);
  } else if (delta_speed / cycle_ < -optimized_accel &&
             std::abs(u_speed) > std::abs(preceding_speed)) { // -制限加速度超過
    u_speed = preceding_speed - (optimized_accel * cycle_);
  }
  // 速度制限
  u_speed = clamp(u_speed, 0.0, max_velocity_);

  u_[0].vx = u_speed * std::cos(u_angle); // 成分速度再計算
  u_[0].vy = u_speed * std::sin(u_angle);

  // 値の更新
  up_[1] = up_[0];
  ui_[1] = ui_[0];
  ud_[1] = ud_[0];
  u_[1]  = u_[0];
  e_[1]  = e_[0];
}

} // controller
} // ai_server