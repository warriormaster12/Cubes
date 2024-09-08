#include "physics-server.h"
#include "logger.h"

#include "LinearMath/btVector3.h"

CuPhysicsServer *CuPhysicsServer::singleton = nullptr;

CuPhysicsServer::CuPhysicsServer() {
  singleton = this;

  collision_config = new btDefaultCollisionConfiguration();
  collision_dispatcher = new btCollisionDispatcher(collision_config);
  broadphase = new btDbvtBroadphase();

  solver = new btSequentialImpulseConstraintSolver();

  dynamic_world = new btDiscreteDynamicsWorld(collision_dispatcher, broadphase,
                                              solver, collision_config);
  dynamic_world->setGravity(btVector3(0.0, 0.0, -9.81));
}

CuPhysicsServer *CuPhysicsServer::get_singleton() { return singleton; }

btCollisionShape *
CuPhysicsServer::create_box_shape(const glm::vec3 &p_half_extents) {
  btBoxShape *shape = new btBoxShape(btVector3(
      btVector3(p_half_extents.x, p_half_extents.y, p_half_extents.z)));
  collision_shapes.push_back(shape);
  return shape;
}

btCollisionObject *
CuPhysicsServer::create_static_body(const btTransform &p_start_transform,
                                    btCollisionShape *p_shape) {
  btCollisionObject *object = new btCollisionObject();
  object->setCollisionShape(p_shape);
  object->setWorldTransform(p_start_transform);
  dynamic_world->addCollisionObject(object);
  return object;
}

btRigidBody *
CuPhysicsServer::create_rigid_body(const float p_mass,
                                   const btTransform &p_start_transform,
                                   btCollisionShape *p_shape) {
  btAssert((!p_shape || p_shape->getShapeType() != INVALID_SHAPE_PROXYTPE));
  bool is_dynamic = p_mass != 0.f;

  btVector3 local_inertia(0, 0, 0);
  if (is_dynamic)
    p_shape->calculateLocalInertia(p_mass, local_inertia);

    // using motionstate is recommended, it provides interpolation capabilities,
    // and only synchronizes 'active' objects

#define USE_MOTIONSTATE 1
#ifdef USE_MOTIONSTATE
  btDefaultMotionState *myMotionState =
      new btDefaultMotionState(p_start_transform);

  btRigidBody::btRigidBodyConstructionInfo cinfo(p_mass, myMotionState, p_shape,
                                                 local_inertia);

  btRigidBody *body = new btRigidBody(cinfo);

#else
  btRigidBody *body = new btRigidBody(p_mass, 0, p_shape, local_inertia);
  body->setWorldTransform(p_start_transform);
#endif //

  body->setUserIndex(-1);
  dynamic_world->addRigidBody(body);
  return body;
}

void CuPhysicsServer::remove_rigid_body(btRigidBody *p_body) {
  dynamic_world->removeRigidBody(p_body);
  btMotionState *ms = p_body->getMotionState();
  delete p_body;
  delete ms;
}

void CuPhysicsServer::remove_collision_shape(const btCollisionShape *p_shape) {
  for (int i = 0; i < collision_shapes.size(); ++i) {
    if (collision_shapes[i] == p_shape) {
      delete p_shape;
      collision_shapes.erase(collision_shapes.begin() + i);
      break;
    }
  }
}

void CuPhysicsServer::remove_static_body(btCollisionObject *p_object) {
  dynamic_world->removeCollisionObject(p_object);
  delete p_object;
}

void step_simulation(btDiscreteDynamicsWorld *p_dynamic_world, double p_delta) {
  CuPhysicsServer *physics = CuPhysicsServer::get_singleton();
  if (!physics) {
    return;
  }
  std::lock_guard<std::mutex> guard(physics->get_physics_mutex());
  p_dynamic_world->stepSimulation(p_delta);
}

void CuPhysicsServer::update_physics(double p_delta) {
  if (!dynamic_world) {
    ENGINE_WARN("No dynamic world setup. Can't update physics");
    return;
  }

  if (physics_thread.joinable()) {
    physics_thread.join();
  }
  physics_thread = std::thread(step_simulation, dynamic_world, p_delta);
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
  if (physics_thread.joinable()) {
    physics_thread.join();
  }
  collision_shapes.clear();
  delete dynamic_world;
  delete solver;
  delete broadphase;
  delete collision_dispatcher;
  delete collision_config;
  singleton = nullptr;
}