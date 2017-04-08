#include "pid_controller.h"

namespace ai_server {
namespace controller {

const double pid_controller::kp_[] = {2.0, 4.0};
const double pid_controller::ki_[] = {0.2, 0.2};
const double pid_controller::kd_[] = {0.0, 0.0};

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
  ep.theta = setpoint.theta - robot.theta();

  // 目標速度
  velocity_t set_velocity;
  set_velocity.vx    = ep.x / cycle_;
  set_velocity.vy    = ep.y / cycle_;
  set_velocity.omega = ep.theta - cycle_;

  // 速度偏差
  e_[0].vx    = set_velocity.vx - robot.vx();
  e_[0].vy    = set_velocity.vy - robot.vy();
  e_[0].omega = set_velocity.omega - robot.omega();

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

  // 値の更新
  up_[1] = up_[0];
  ui_[1] = ui_[0];
  ud_[1] = ud_[0];
  u_[1]  = u_[0];
  e_[1]  = e_[0];
}

} // controller
} // ai_server
