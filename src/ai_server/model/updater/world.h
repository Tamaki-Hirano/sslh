#ifndef AI_SERVER_MODEL_UPDATER_WORLD_H
#define AI_SERVER_MODEL_UPDATER_WORLD_H

#include <Eigen/Geometry>

#include "ai_server/model/world.h"
#include "ball.h"
#include "field.h"
#include "robot.h"

// 前方宣言
namespace ssl_protos {
namespace vision {
class Packet;
}
}

namespace ai_server {
namespace model {
namespace updater {

class world {
  /// フィールドのupdater
  field field_;
  /// ボールのupdater
  ball ball_;
  /// 青ロボットのupdater
  robot<model::team_color::blue> robots_blue_;
  /// 黄ロボットのupdater
  robot<model::team_color::yellow> robots_yellow_;

public:
  world()             = default;
  world(const world&) = delete;
  world& operator=(const world&) = delete;

  /// @brief                  内部の状態を更新する
  /// @param packet           SSL-Visionのパース済みパケット
  void update(const ssl_protos::vision::Packet& packet);

  /// @brief           値を取得する
  model::world value() const;

  /// @brief           updaterに変換行列を設定する
  /// @param matrix    変換行列
  void set_transformation_matrix(const Eigen::Affine3d& matrix);
  /// @brief           updaterに変換行列を設定する
  /// @param x         x軸方向に平行移動する量
  /// @param y         y軸方向に平行移動する量
  /// @param theta     z軸を中心に回転する量
  void set_transformation_matrix(double x, double y, double theta);

  /// @brief           フィールドのupdaterを取得する
  field& field_updater();
  /// @brief           ボールのupdaterを取得する
  ball& ball_updater();
  /// @brief           青ロボットのupdaterを取得する
  robot<model::team_color::blue>& robots_blue_updater();
  /// @brief           黄ロボットのupdaterを取得する
  robot<model::team_color::yellow>& robots_yellow_updater();
};

} // namespace updater
} // namespace model
} // namespace ai_server

#endif // AI_SERVER_MODEL_UPDATER_WORLD_H