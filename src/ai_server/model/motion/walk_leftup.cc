#include "walk_leftup.h"

namespace ai_server::model::motion {

walk_leftup::walk_leftup() : base(520) {}

std::tuple<double, double, double> walk_leftup::execute() {
  return std::make_tuple<double, double, double>(0.0, -150.0, 0.0);
}

} // namespace ai_server::model::motion