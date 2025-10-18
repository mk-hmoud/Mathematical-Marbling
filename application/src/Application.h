#ifndef APPLICATION_H
#define APPLICATION_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Canvas.h"
#include "Renderer.h"

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
    Renderer _renderer;

    // Input state
    bool _tineMode;
    bool _isDragging;
    double _startX, _startY;
};

#endif // APPLICATION_h