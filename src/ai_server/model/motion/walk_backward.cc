#include "walk_backward.h"

namespace ai_server::model::motion {

walk_backward::walk_backward() : base(2) {}

std::tuple<double, double, double> walk_backward::execute() {
  return std::make_tuple<double, double, double>(-100.0, 0.0, 0.0);
}

} // namespace ai_server::model::motion