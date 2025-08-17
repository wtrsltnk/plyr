#ifndef PHYSICSSERVICE_H
#define PHYSICSSERVICE_H

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <LinearMath/btIDebugDraw.h>
#include <btBulletDynamicsCommon.h>
#include <chrono>
#include <glm/glm.hpp>
#include <iphysicsservice.hpp>

class PhysicsService : public IPhysicsService
{
public:
    PhysicsService();

    virtual ~PhysicsService();

    virtual void Step(
        std::chrono::nanoseconds diff);

    virtual PhysicsComponent AddCube(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos);

    virtual void ApplyForce(
        const PhysicsComponent &component,
        const glm::vec3 &force,
        const glm::vec3 &relativePosition = glm::vec3());

    virtual glm::mat4 GetMatrix(
        const PhysicsComponent &component);

    virtual void RenderDebug(
        VertexArray &vertexAndColorBuffer);

private:
    btBroadphaseInterface *mBroadphase = nullptr;
    btDefaultCollisionConfiguration *mCollisionConfiguration = nullptr;
    btCollisionDispatcher *mDispatcher = nullptr;
    btSequentialImpulseConstraintSolver *mSolver = nullptr;
    btDiscreteDynamicsWorld *mDynamicsWorld = nullptr;
    std::vector<btRigidBody *> _rigidBodies;

    PhysicsComponent AddObject(
        btCollisionShape *shape,
        float mass,
        const glm::vec3 &startPos,
        bool allowDeactivate);
};

#endif // PHYSICSSERVICE_H
