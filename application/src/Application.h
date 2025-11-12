#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Canvas.h"
#include "Renderer.h"

class Renderer;

class Application
{
public:
    Application(int width, int height, const char *title);
    ~Application();
    void run();

private:
    void handleMouseInput(int button, int action, int mods);
    void handleKeyInput(int key, int scancode, int action, int mods);

private:
    GLFWwindow *_window;
    Canvas _simulation;
    std::unique_ptr<Renderer> _renderer;

    // Input state
    bool _tineMode;
    bool _isDragging;
    double _startX, _startY;

    // GUI state
    float _vortexStrength = 200.0f;
    float _vortexRadius = 50.0f;
    float _dropRadius = 75.0f;
    Color _nextDropColor = {1.0f, 0.2f, 0.2f};
};

#endif // APPLICATION_H