#include "kickl_forward.h"

 namespace ai_server::model::motion {

  kickl_forward::kickl_forward() : base(26){}

  std::tuple<double,double,double> kickl_forward::execute() {
     return std::make_tuple<double,double,double>(100.0, 0.0, 0.0);

  }
 }