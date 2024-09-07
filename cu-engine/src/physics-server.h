#pragma once

#include "btBulletDynamicsCommon.h"
#include <vector>

class CuPhysicsServer {
public:
  CuPhysicsServer();
  ~CuPhysicsServer();
  static CuPhysicsServer *get_singleton();

  void update_physics(double p_delta);

private:
  static CuPhysicsServer *singleton;

  btDefaultCollisionConfiguration *collision_config = nullptr;
  btCollisionDispatcher *collision_dispatcher = nullptr;
  btDbvtBroadphase *broadphase = nullptr;
  btSequentialImpulseConstraintSolver *solver = nullptr;
  btDiscreteDynamicsWorld *dynamic_world = nullptr;
  std::vector<btCollisionShape *> collision_shapes;
};