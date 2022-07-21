// pos 表示　2022　0707
#include <cmath>
#include "clear.h"
#include "ai_server/util/math/angle.h"
#include "ai_server/util/math/to_vector.h"
#include "ai_server/model/motion/walk_forward.h"
#include "ai_server/model/motion/turn_left.h"
#include "ai_server/model/motion/turn_right.h"

#include <boost/geometry/geometry.hpp>              //mw
#include <boost/geometry/geometries/segment.hpp>    //mw
#include <boost/math/constants/constants.hpp>       //mw
#include "ai_server/util/math/geometry.h"           //mw
#include "ai_server/util/math/distance.h"
#include <iostream> //omega表示のため


namespace ai_server::game::action {
    clear::clear(context& ctx, unsigned int id) : base(ctx, id){}
    bool clear::finished() const{
        return false;
    }

model::command clear::execute() {
    //const double omega = 2.0;
    const Eigen::Vector2d our_goal_pos(world().field().x_min(), 0.0);   // mw target0
    const Eigen::Vector2d ene_goal_pos(world().field().x_max(), 0.0);   // mw target0
    const auto target_= ene_goal_pos;                                   // mw target0
  
    //const Eigen::Vector2d target_{2000.0,0.0};      // mw target1

    model::command command{};
    const auto our_robots =model::our_robots(world(), team_color());
    if(!our_robots.count(id_)) return command;
    const auto robot = our_robots.at(id_);
    
    const auto robot_pos =util::math::position(robot);
    const auto ball_pos =util::math::position(world().ball());
    const auto Robo_theta = robot.theta(); 
    //const auto Robo_theta = util::math::wrap_to_pi(robot.theta());                               // mw
    
    auto rad = 90;
    Eigen::Vector2d pos = ball_pos + rad * (ball_pos - target_).normalized();

    const auto pos_robot = util::math::distance(pos,robot_pos);
    if(30 >= pos_robot){pos = ball_pos;} // mw
    //if(20 < (util::math::distance(ball_pos,robot_pos))){pos = ball_pos;} // mw

    
    //auto omega =  util::math::inferior_angle(robot.theta(),  // mw xy空間において、小さい方の角度差をとり、その絶対値を返す
    //                                 util::math::direction(pos,robot_pos));// util::math::direction(ball_pos,robot_pos));       //mw
    auto omega =  util::math::direction_from(util::math::direction(pos,robot_pos),robot.theta());
    
    std::cout << "pos " << pos << "\n";                          // mw
    std::cout << "r_theat " << Robo_theta << "\n";                // mw 
    std::cout << "omega " << omega << "\n";                     // mv

/*constexpr double rot_th0 = 0.2; 
 if(rot_th0 > omega || omega > -rot_th0){  
 command.set_motion(std::make_shared<model::motion::walk_forward>());}
*/
command.set_motion(std::make_shared<model::motion::walk_forward>());

constexpr double rot_th = 0.3; 
                        
if(rot_th < omega){
    command.set_motion(std::make_shared<model::motion::turn_left>());
 //   command.set_motion(std::make_shared<model::motion::walk_forward>()); この位置では直進のみ、旋回コマンドが出力された直後直進のため
    }else if(omega < -rot_th){
    command.set_motion(std::make_shared<model::motion::turn_right>());
 //   command.set_motion(std::make_shared<model::motion::walk_forward>()); この位置では直進のみ、旋回コマンドが出力された直後直進のため
    } 

/*constexpr double rot_th0 = 0.1;  // この位置では旋回のみで前進しない
 if(rot_th0 > omega && omega > -rot_th0){    
command.set_motion(std::make_shared<model::motion::walk_forward>());} //この位置では直進のみ、旋回コマンドが出力された直後直進のため 
*/  
    /*else{ //こっから先未知もし回転が終わったのならゴールまで行く。
    
     auto omega2 =  util::math::inferior_angle(robot.theta(),
                                     util::math::direction(target_,robot_pos));
    
       if(rot_th < omega2){
            command.set_motion(std::make_shared<model::motion::turn_left>());
            }else if(omega2 < -rot_th){
            command.set_motion(std::make_shared<model::motion::turn_right>());
            }
        command.set_motion(std::make_shared<model::motion::walk_forward>());
        
     //ここまで付け足しだけど、リターン動かしすぎてわからなくなっちゃった
     //mw 上記プログラムは最初のif文でpos方向に旋回して、更にゴール方向の旋回します。
     // まず　フローを書きましょう、
     // 1.ボールとロボットの位置または角度から、回り込むか押し込むか判断します。
     // 2.押し込み位置の場合、posにそのまま進み押し込みます。（このプログラムが基本です。）
     // 3.posに移動後posをボール位置に変更しないと、ロボットとposの位置が重なるため旋回します。
     // 4.
     
    
    }*/
return command;   
//command.set_motion(std::make_shared<model::motion::walk_forward>());
}

 
}