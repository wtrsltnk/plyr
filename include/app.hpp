#ifndef APP_H
#define APP_H

#include <chrono>
#include <glm/glm.hpp>
#include <glprogram.hpp>
#include <iphysicsservice.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <vertexarray.hpp>

class App
{
public:
    App(const std::vector<std::string> &args);
    virtual ~App();

    bool Init();
    int Run();

    void OnInit();
    void OnFrame(std::chrono::nanoseconds diff);
    void OnResize(int width, int height);
    void OnExit();

    template <class T>
    T *GetWindowHandle() const;

protected:
    const std::vector<std::string> &_args;
    glm::mat4 _projection;
    std::unique_ptr<GlProgram> _program;
    std::unique_ptr<VertexArray> _vertexArray;
    entt::registry _registry;
    std::unique_ptr<IPhysicsService> _physics;

    entt::entity AddObject(
        float mass,
        const glm::vec3 &size,
        const glm::vec3 &startPos);

    template <class T>
    void SetWindowHandle(T *handle);

    void ClearWindowHandle();

    static float vertices[216];

private:
    void *_windowHandle;
};

#endif // APP_H
