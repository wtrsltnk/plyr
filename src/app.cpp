#include <app.hpp>

#include "services/physicsservice.hpp"
#include <entities.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glprogram.hpp>
#include <glshader.hpp>
#include <imgui.h>
#include <iostream>

void App::OnInit()
{
    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    _program = std::unique_ptr<GlProgram>(new GlProgram());
    _vertexArray = std::make_unique<VertexArray>();
    _physics = std::make_unique<PhysicsService>();

    _program->attach(GLSL_VERTEX_SHADER(
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;

        out vec3 Color;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0f);
            Color = aColor;
        }));

    _program->attach(GLSL_FRAGMENT_SHADER(
        out vec4 FragColor;

        in vec3 Color;

        void main() {
            FragColor = vec4(Color.rgb, 1.0);
        }));

    _program->link();

    AddObject(0.0f, glm::vec3(15.0f, 15.0f, 0.4f), glm::vec3(0.0f, 0.0f, -3.0f));

    auto a = AddObject(0.01f, glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 12.0f));
    _physics->ApplyForce(_registry.get<PhysicsComponent>(a), glm::vec3(0.0f, -1.0f, 0.3f), glm::vec3(0.0f, 1.0f, 0.5f));

    auto b = AddObject(0.01f, glm::vec3(3.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    _physics->ApplyForce(_registry.get<PhysicsComponent>(b), glm::vec3(0.0f, -1.0f, -0.3f), glm::vec3(0.0f, 1.0f, 0.5f));

    _vertexArray->upload();
}

entt::entity App::AddObject(
    float mass,
    const glm::vec3 &size,
    const glm::vec3 &startPos)
{
    auto entity = _registry.create();

    auto component = _physics->AddCube(mass, size, startPos);

    auto mesh = _vertexArray->add(vertices, 36, size);

    _registry.emplace<PhysicsComponent>(entity, component);
    _registry.emplace<MeshComponent>(entity, std::get<0>(mesh), std::get<1>(mesh));

    return entity;
}

void App::OnResize(
    int width,
    int height)
{
    if (height < 1) height = 1;

    glViewport(0, 0, width, height);

    _projection = glm::perspective(glm::radians<float>(90), float(width) / float(height), 0.1f, 1000.0f);
}

void App::OnFrame(
    std::chrono::nanoseconds diff)
{
    _physics->Step(diff);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _program->use();

    auto viewMatrix = glm::lookAt(
        glm::vec3(10.0f, -10.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    _program->setUniformMatrix("view", viewMatrix);
    _program->setUniformMatrix("projection", _projection);

    auto objects = _registry.view<PhysicsComponent, MeshComponent>();
    for (auto entity : objects)
    {
        auto &body = objects.get<PhysicsComponent>(entity);
        auto &mesh = objects.get<MeshComponent>(entity);

        auto mat = _physics->GetMatrix(body);
        _program->setUniformMatrix("model", mat);

        _vertexArray->render(RenderModes::Triangles, mesh.first, mesh.count);
    }

    static bool showPhysicsDebugMode = true;

    if (showPhysicsDebugMode)
    {
        VertexArray vertexAndColorBuffer;

        _physics->RenderDebug(vertexAndColorBuffer);

        vertexAndColorBuffer.upload();

        glDisable(GL_DEPTH_TEST);
        _program->setUniformMatrix("model", glm::mat4(1.0f));
        vertexAndColorBuffer.render(RenderModes::Lines, 0);
        glEnable(GL_DEPTH_TEST);
    }

    ImGui::Begin("Debug");
    ImGui::Checkbox("Enabled debug draw", &showPhysicsDebugMode);
    ImGui::End();
}

void App::OnExit()
{
    _program = nullptr;
    _vertexArray = nullptr;
}

// pos x, y, z, color h, s, v
float App::vertices[216] = {
    -0.5f, -0.5f, -0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.4f, 0.6f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.4f, 0.6f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.4f, 0.6f, 1.0f,

    -0.5f, -0.5f, 0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.4f, 0.6f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.4f, 0.6f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.4f, 0.6f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.4f, 0.6f, 1.0f,

    -0.5f, 0.5f, 0.5f, 0.5f, 0.6f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.5f, 0.6f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.5f, 0.6f, 1.0f,

    0.5f, 0.5f, 0.5f, 0.5f, 0.6f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.5f, 0.6f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.5f, 0.6f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.5f, 0.6f, 1.0f,

    -0.5f, -0.5f, -0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.6f, 0.6f, 1.0f,

    -0.5f, 0.5f, -0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.6f, 0.6f, 1.0f};
