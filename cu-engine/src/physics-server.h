#pragma once

#include "btBulletDynamicsCommon.h"
#include <glm.hpp>
#include <vector>

class CuPhysicsServer {
public:
  CuPhysicsServer();
  ~CuPhysicsServer();
  static CuPhysicsServer *get_singleton();

  btCollisionShape *create_box_shape(const glm::vec3 &p_half_extents);
  btCollisionObject *create_static_body(const btTransform &p_start_transform,
                                        btCollisionShape *p_shape);
  btRigidBody *create_rigid_body(const float p_mass,
                                 const btTransform &p_start_transform,
                                 btCollisionShape *p_shape);
  void remove_rigid_body(btRigidBody *p_body);
  void remove_collision_shape(const btCollisionShape *p_shape);

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