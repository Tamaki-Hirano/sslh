#ifndef AI_SERVER_MODEL_MOTION_KICKL_FORWERD_H
#define AI_SERVER_MODEL_MOTION_KICKL_FORWARD_H

#include "base.h"

namespace ai_server::model::motion {

 class kick_forward : public base 
 {
 
 public:
  kick_forward(/* args */);
 
 std::tuple<double,double,double> execute() override;
 
 };

}

#endif
 
