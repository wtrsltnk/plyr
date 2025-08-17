#ifndef IPHYSICSSERVICE_H
#define IPHYSICSSERVICE_H

#include <chrono>
#include <entities.hpp>
#include <glm/glm.hpp>
#include <vertexarray.hpp>

class IPhysicsService
{
public:
    virtual ~IPhysicsService() = default;

    virtual void Step(
        std::chrono::nanoseconds diff) = 0;

    virtual PhysicsComponent AddCube(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos) = 0;

    virtual void ApplyForce(
        const PhysicsComponent &component,
        const glm::vec3 &force,
        const glm::vec3 &relativePosition = glm::vec3()) = 0;

    virtual glm::mat4 GetMatrix(
        const PhysicsComponent &component) = 0;

    virtual void RenderDebug(
        VertexArray &vertexAndColorBuffer) = 0;
};

#endif // IPHYSICSSERVICE_H
