
/*
#include <cmath>
#include "clear.h"
#include "ai_server/util/math/angle.h"
#include "ai_server/util/math/to_vector.h"
#include "ai_server/model/motion/walk_forward.h"
#include "ai_server/model/motion/turn_left.h"
#include "ai_server/model/motion/turn_right.h"

namespace ai_server::game::action {
    clear::clear(context& ctx, unsigned int id) : base(ctx, id){}
    bool clear::finished() const{
        return false;
    }

model::command clear::execute() {
    const double omega = 2.0;
    model::command command{};
    const auto our_robots =model::our_robots(world(), team_color());
    if(!our_robots.count(id_)) return command;
    const auto robot = our_robots.at(id_);
    
    const auto robot_pos =util::math::position(robot);
    const auto ball_pos =util::math::position(world().ball());

command.set_motion(std::make_shared<model::motion::walk_forward>());
constexpr double rot_th = 0.5;
if(rot_th <
    util::math::inferior_angle(robot.theta(),
        util::math::direction(ball_pos,robot_pos))){
    command.set_motion(std::make_shared<model::motion::turn_left>());
    }else if(omega < -rot_th){
    command.set_motion(std::make_shared<model::motion::turn_right>());
    }
    return command;
}
}
*/

/*#include<cmath>
#include "clear.h"
#include "ai_server/util/math/angle.h"
#include "ai_server/util/math/to_vector.h"
#include "ai_server/model/motion/walk_forkward.h"
#include "ai-server/model/motion/turn_left.h"
#include "ai-server/model/motion/turn_right.h

namespace ai_server::game::action{
    clear::clear(context& ctx,unsigned int id) : base(ctx,id){}
    bool clear:: finishied()const{
           return false;
    }

   
    model::command clear::excute(){
        model::command{};

        const auto our_robots =model::our_robots(world(),team_color());
        if(!our_robots.count(id_))return command;

        const auto robots.at(id_)
        const auto robot_pos = util::math::position(robot);
        const auto ball_pos = util::math::position(world().ball());


        command.set_motion(std::make_shared<model::motion::walk_forward>());
        constexpr double rot_th = 0.5;
        if(rot_th<
        util::math::inferior_angle(robot.theta(),
        util::math::direction(ball_pos)))
*/
// hirano
    #include <cmath>
#include "clear.h"
#include "ai_server/util/math/angle.h"
#include "ai_server/util/math/to_vector.h"
#include "ai_server/model/motion/walk_forward.h"
#include "ai_server/model/motion/turn_left.h"
#include "ai_server/model/motion/turn_right.h"

#include <iostream> //22,6,29追記オメガ表示のため
namespace ai_server::game::action {
clear::clear(context& ctx, unsigned int id) : base(ctx, id) {}
bool clear::finished() const {
// 終了条件は設定しないため常にfalse
return false;
}
model::command clear::execute() {
model::command command{};
// 自チームのロボットの情報を取得
const auto our_robots = model::our_robots(world(), team_color());
// 自分の情報がなければ終了
auto omega = 2.0;
if (!our_robots.count(id_)) return command;
// 自分の情報を取得
const auto robot = our_robots.at(id_);
// 自分とボールの位置を取得
const auto robot_pos = util::math::position(robot);
const auto ball_pos = util::math::position(world().ball());
// 前進
command.set_motion(std::make_shared<model::motion::walk_forward>());
// 向きが合っていなければ回転(前進のモーションはキャンセルされる)
constexpr double rot_th = 0.5;
if (rot_th <
util::math::inferior_angle(robot.theta(),
util::math::direction(ball_pos, robot_pos))) {
command.set_motion(std::make_shared<model::motion::turn_left>());
} else if (omega < -rot_th) {
command.set_motion(std::make_shared<model::motion::turn_right>());
}
std::cout << "omega" <<  omega << "\n" ; // 22,6,29 omega表示
return command;
}
} // namespace ai_server::game::action




 