#include "walk_forwardup.h"

namespace ai_server::model::motion {

walk_forwardup::walk_forwardup() : base(513) {}

std::tuple<double, double, double> walk_forwardup::execute() {
  return std::make_tuple<double, double, double>(150.0, 0.0, 0.0);
}

} // namespace ai_server::model::motion