#include "physicsservice.hpp"

#include <entities.hpp>
#include <glm/gtc/type_ptr.hpp>

PhysicsService::PhysicsService()
{
    mCollisionConfiguration = new btDefaultCollisionConfiguration();
    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

    mBroadphase = new btDbvtBroadphase();

    mSolver = new btSequentialImpulseConstraintSolver();

    mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);
    mDynamicsWorld->setGravity(btVector3(0, 0, -9.81f));
    mDynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
}

PhysicsService::~PhysicsService() = default;

void PhysicsService::Step(
    std::chrono::nanoseconds diff)
{
    static double time = 0;
    time += static_cast<double>(diff.count() / 1000000.0);
    while (time > 0)
    {
        mDynamicsWorld->stepSimulation(btScalar(1000.0f / 60.0f), btScalar(1.0) );

        time -= (1000.0 / 60.0);
    }
}

PhysicsComponent PhysicsService::AddObject(
    btCollisionShape *shape,
    float mass,
    const glm::vec3 &startPos,
    bool disableDeactivation)
{
    btVector3 fallInertia(0, 0, 0);

    shape->calculateLocalInertia(mass, fallInertia);

    btRigidBody::btRigidBodyConstructionInfo rbci(mass, nullptr, shape, fallInertia);
    rbci.m_startWorldTransform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));

    auto body = new btRigidBody(rbci);
    if (disableDeactivation)
    {
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    auto bodyIndex = _rigidBodies.size();

    mDynamicsWorld->addRigidBody(body);
    _rigidBodies.push_back(body);

    return PhysicsComponent({
        bodyIndex,
    });
}

PhysicsComponent PhysicsService::AddCube(
    float mass,
    const glm::vec3 &size,
    const glm::vec3 &startPos)
{
    auto shape = new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));

    return AddObject(shape, mass, startPos, true);
}

void PhysicsService::ApplyForce(
    const PhysicsComponent &component,
    const glm::vec3 &force,
    const glm::vec3 &relativePosition)
{
    _rigidBodies[component.bodyIndex]->applyForce(
        btVector3(force.x, force.y, force.z),
        btVector3(relativePosition.x, relativePosition.y, relativePosition.z));
}

glm::mat4 PhysicsService::GetMatrix(
    const PhysicsComponent &component)
{
    glm::mat4 mat;

    _rigidBodies[component.bodyIndex]->getWorldTransform().getOpenGLMatrix(glm::value_ptr(mat));

    return mat;
}

class GLDebugDrawer : public btIDebugDraw
{
public:
    GLDebugDrawer(
        btDiscreteDynamicsWorld *dynamicsWorld,
        VertexArray &vertexAndColorBuffer);

    virtual ~GLDebugDrawer();

    virtual void drawLine(
        const btVector3 &from,
        const btVector3 &to,
        const btVector3 &color);

    virtual void drawContactPoint(
        const btVector3 &PointOnB,
        const btVector3 &normalOnB,
        btScalar distance,
        int lifeTime,
        const btVector3 &color);

    virtual void reportErrorWarning(
        const char *warningString);

    virtual void draw3dText(
        const btVector3 &location,
        const char *textString);

    virtual void setDebugMode(
        int debugMode);

    virtual int getDebugMode() const { return m_debugMode; }

private:
    int m_debugMode = 0;
    btDiscreteDynamicsWorld *_dynamicsWorld = nullptr;
    VertexArray &_vertexAndColorBuffer;
};

void PhysicsService::RenderDebug(
    VertexArray &vertexAndColorBuffer)
{
    GLDebugDrawer glDebugDrawer(
        mDynamicsWorld,
        vertexAndColorBuffer);

    mDynamicsWorld->debugDrawWorld();
}

#include <glad/glad.h>

GLDebugDrawer::GLDebugDrawer(
    btDiscreteDynamicsWorld *dynamicsWorld,
    VertexArray &vertexAndColorBuffer)
    : m_debugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb),
      _dynamicsWorld(dynamicsWorld),
      _vertexAndColorBuffer(vertexAndColorBuffer)
{
    _dynamicsWorld->setDebugDrawer(this);
}

GLDebugDrawer::~GLDebugDrawer()
{
    _dynamicsWorld->setDebugDrawer(nullptr);
}

void GLDebugDrawer::drawLine(
    const btVector3 &from,
    const btVector3 &to,
    const btVector3 &color)
{
    float lineData[] = {
        from.getX(),
        from.getY(),
        from.getZ(),
        color.getX(),
        color.getY(),
        color.getZ(),
        to.getX(),
        to.getY(),
        to.getZ(),
        color.getX(),
        color.getY(),
        color.getZ(),
    };

    _vertexAndColorBuffer.add(lineData, 2);
}

void GLDebugDrawer::setDebugMode(
    int debugMode)
{
    m_debugMode = debugMode;
}

void GLDebugDrawer::draw3dText(
    const btVector3 &,
    const char *)
{}

void GLDebugDrawer::reportErrorWarning(
    const char *warningString)
{
    printf("%s", warningString);
}

void GLDebugDrawer::drawContactPoint(
    const btVector3 &,
    const btVector3 &,
    btScalar,
    int,
    const btVector3 &)
{}
