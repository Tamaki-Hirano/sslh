
#ifndef AI_SERVER_GAME_ACTION_CLEAR_H
#define AI_SERVER_GAME_ACTION_CLEAR_H

#include <Eigen/Core>
#include "base.h"
//#include <vector>
//#include "ai_server/model/world.h"

namespace ai_server::game::action{

    class clear : public base {
        public:
        clear(context& ctx, unsigned int id);
        model::command execute() override;
        bool finished() const override;

        
    };
}
    #endif


/*
#ifndef AI_SERVER_GAME_ACTION_CLEAR_H
#define AI_SERVER_GAME_ACTION_CLEAR_H

#include <Eigen/Core>
#include "base.h"
//#include "ai_server/game/action/move_2walk.h"

namespace ai_server::game::action{

class clear : public base {
    public:
     clear(context& ctx, unsigned int id);
    model::command execute() override;
    bool finished() const override;

    private:
//    game::action::move_2walk move_2walk_;
};

}
#endif
*/
/*
#ifndef AI_SERVER_GAME_ACTION_CLEAR_H
#define AI_SERVER_GAME_ACTION_CLEAR_H

#include <Eigen/Core>
#include "base.h"

namespace ai_server::game::action{

    class clear : public base {
        public:
        clear(context& ctx, unsigned int id);
        model::command excute() override;
        bool finished() const override;

        orivate:
    };
}
    #endif
    */
