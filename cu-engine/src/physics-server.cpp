#include "physics-server.h"
#include "logger.h"

CuPhysicsServer *CuPhysicsServer::singleton = nullptr;

CuPhysicsServer::CuPhysicsServer() {
  singleton = this;

  collision_config = new btDefaultCollisionConfiguration();
  collision_dispatcher = new btCollisionDispatcher(collision_config);
  broadphase = new btDbvtBroadphase();

  solver = new btSequentialImpulseConstraintSolver();

  dynamic_world = new btDiscreteDynamicsWorld(collision_dispatcher, broadphase,
                                              solver, collision_config);
}

void CuPhysicsServer::update_physics(double p_delta) {
  if (!dynamic_world) {
    ENGINE_WARN("No dynamic world setup. Can't update physics");
    return;
  }

  dynamic_world->stepSimulation(p_delta);
}

CuPhysicsServer::~CuPhysicsServer() {
  if (dynamic_world) {
    for (int i = 0; i < dynamic_world->getNumConstraints(); ++i) {
      dynamic_world->removeConstraint(dynamic_world->getConstraint(i));
    }

    for (int i = 0; i < dynamic_world->getNumCollisionObjects(); ++i) {
      btCollisionObject *obj = dynamic_world->getCollisionObjectArray()[i];
      btRigidBody *body = btRigidBody::upcast(obj);
      if (body && body->getMotionState()) {
        delete body->getMotionState();
      }
      dynamic_world->removeCollisionObject(obj);
      delete obj;
    }
  }

  for (int i = 0; i < collision_shapes.size(); ++i) {
    btCollisionShape *shape = collision_shapes[i];
    delete shape;
  }
  collision_shapes.clear();
  delete dynamic_world;
  delete solver;
  delete broadphase;
  delete collision_dispatcher;
  delete collision_config;
  singleton = nullptr;
}