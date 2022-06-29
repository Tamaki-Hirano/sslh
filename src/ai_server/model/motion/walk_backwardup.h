#ifndef AI_SERVER_MODEL_MOTION_WALK_BACKWARDUP_H
#define AI_SERVER_MODEL_MOTION_WALK_BACKWARDUP_H

#include "base.h"

namespace ai_server::model::motion {

class walk_backwardup : public base {
public:
  walk_backwardup();

  std::tuple<double, double, double> execute() override;
};

} // namespace ai_server::model::motion

#endif