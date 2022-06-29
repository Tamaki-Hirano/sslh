#include "walk_right.h"

namespace ai_server::model::motion {

walk_right::walk_right() : base(4) {}

std::tuple<double, double, double> walk_right::execute() {
  return std::make_tuple<double, double, double>(0.0, 100.0, 0.0);
}

} // namespace ai_server::model::motion