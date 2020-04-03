#include <limits>
#include "base.h"

namespace ai_server::planner {

using double_limits = std::numeric_limits<double>;

base::base()
    : max_pos_(double_limits::max(), double_limits::max()),
      min_pos_(double_limits::lowest(), double_limits::lowest()),
      margin_(120.0) {}

base::~base() {}

void base::set_max_pos(const Eigen::Vector2d& max_p) {
  max_pos_ = max_p;
}

void base::set_min_pos(const Eigen::Vector2d& min_p) {
  min_pos_ = min_p;
}

void base::set_margin(double m) {
  margin_ = m;
}
} // namespace ai_server::planner
