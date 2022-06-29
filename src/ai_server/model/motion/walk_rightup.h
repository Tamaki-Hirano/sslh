#ifndef AI_SERVER_MODEL_MOTION_WALK_RIGHTUP_H
#define AI_SERVER_MODEL_MOTION_WALK_RIGHTUP_H

#include "base.h"

namespace ai_server::model::motion {

class walk_rightup : public base {
public:
  walk_rightup();

  std::tuple<double, double, double> execute() override;
};

} // namespace ai_server::model::motion

#endif