#include "walk_rightup.h"

namespace ai_server::model::motion {

walk_rightup::walk_rightup() : base(516) {}

std::tuple<double, double, double> walk_rightup::execute() {
  return std::make_tuple<double, double, double>(0.0, 150.0, 0.0);
}

} // namespace ai_server::model::motion