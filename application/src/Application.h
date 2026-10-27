#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Artwork.h"
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
    void addRandomDrop();

private:
    GLFWwindow *_window;
    Artwork _artwork;
    std::unique_ptr<Renderer> _renderer;

    // Input state
    bool _isDragging;
    double _startX, _startY;

    enum class ToolMode
    {
        Drop,
        Tine,
        Comb,
        Vortex
    };
    ToolMode _toolMode = ToolMode::Drop;

    VortexSettings _vortexSettings;
    TineSettings _tineSettings;
    CombSettings _combSettings;

    // GUI state
    float _dropRadius = 75.0f;
    Color _nextDropColor = {1.0f, 0.2f, 0.2f};
    Color _backgroundColor = {0.07f, 0.07f, 0.09f};
};

#endif // APPLICATION_H