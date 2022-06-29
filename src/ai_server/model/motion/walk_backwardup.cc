#include "walk_backwardup.h"

namespace ai_server::model::motion {

walk_backwardup::walk_backwardup() : base(514) {}

std::tuple<double, double, double> walk_backwardup::execute() {
  return std::make_tuple<double, double, double>(-150.0, 0.0, 0.0);
}

} // namespace ai_server::model::motion