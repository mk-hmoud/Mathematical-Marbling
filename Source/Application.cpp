#include "Application.h"
#include <iostream>

Application::Application(int width, int height, const char* title)
    : canvas(width, height){
    if (!glfwInit()) {
        return;
    }
    
    glfwWindowHint(GLFW_SAMPLES, 8); //MSAA (Multi Sample Anti Aliasing)
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glfwSetWindowUserPointer(window, this);

    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    glEnable(GL_MULTISAMPLE);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        auto app = static_cast<Application*>(glfwGetWindowUserPointer(win));
        app->handleMouseInput(button, action, mods);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        auto app = static_cast<Application*>(glfwGetWindowUserPointer(win));
        app->handleKeyInput(key, action, mods);
    });
}

void Application::handleMouseInput(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        canvas.add_drop(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void Application::handleKeyInput(int key, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    xpos = static_cast<float>(xpos);
    ypos = static_cast<float>(ypos);
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_T:
                canvas.tine((int)xpos, (int)ypos, 80, 16);
                break;
            case GLFW_KEY_BACKSPACE:
                canvas.clear();
                break;
            case GLFW_KEY_V:
                canvas.vortex(xpos, ypos, 200.0f, 60.0f, 50.0f);
                break;
            case GLFW_KEY_P:
                canvas.addDropsWithPerlinNoise(100, 5.0f, 12.0f, 2.0f);
                break;
            case GLFW_KEY_S:
                break;
            default:
                break;
        }
    }
}

Application::~Application() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        canvas.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}