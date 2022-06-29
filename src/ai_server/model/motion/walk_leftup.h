#ifndef AI_SERVER_MODEL_MOTION_WALK_LEFTUP_H
#define AI_SERVER_MODEL_MOTION_WALK_LEFTUP_H

#include "base.h"

namespace ai_server::model::motion {

class walk_leftup : public base {
public:
  walk_leftup();

  std::tuple<double, double, double> execute() override;
};

} // namespace ai_server::model::motion

#endif