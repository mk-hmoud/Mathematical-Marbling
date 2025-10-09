#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Canvas.h"

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void run();
private:
    void handleMouseInput(int button, int action, int mods);
    void handleKeyInput(int key, int action, int mods);

    GLFWwindow* window;
    Canvas canvas;
    bool tineMode;
    int startX, startY, endX, endY;
};
