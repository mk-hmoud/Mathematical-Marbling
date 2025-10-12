#include "Application.h"

#include <iostream>

Application::Application(int width, int height, const char *title)
    : _simulation(width, height), _tineMode(false), _isDragging(false), _startX(0), _startY(0)
{
    if (!glfwInit())
    {
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 8); // MSAA (Multi Sample Anti Aliasing)
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window)
    {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(_window);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glfwSetWindowUserPointer(_window, this);

    if (glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
        return;
    }

    glEnable(GL_MULTISAMPLE);

    glfwSetMouseButtonCallback(_window, [](GLFWwindow *win, int button, int action, int mods)
                               {
        auto app = static_cast<Application*>(glfwGetWindowUserPointer(win));
        app->handleMouseInput(button, action, mods); });

    glfwSetKeyCallback(_window, [](GLFWwindow *win, int key, int scancode, int action, int mods)
                       {
        auto app = static_cast<Application*>(glfwGetWindowUserPointer(win));
        app->handleKeyInput(key, action, mods); });
}

void Application::handleMouseInput(int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    // cursor position for both modes
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);

    if (_tineMode)
    {
        if (action == GLFW_PRESS)
        {
            _isDragging = true;
            _startX = xpos;
            _startY = ypos;
        }
        else if (action == GLFW_RELEASE)
        {
            if (_isDragging)
            {
                _simulation.tine(Point(_startX, _startY), Point(xpos, ypos), 80, 16);
                _isDragging = false;
            }
        }
    }
    else
    {
        if (action == GLFW_PRESS)
        {
            _simulation.add_drop(static_cast<float>(xpos), static_cast<float>(ypos));
        }
    }
}

void Application::handleKeyInput(int key, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    xpos = static_cast<float>(xpos);
    ypos = static_cast<float>(ypos);
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_T:
            _tineMode = !_tineMode;
            _isDragging = false;
            std::cout << "Tine Mode: " << (_tineMode ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_BACKSPACE:
            _simulation.clear();
            break;
        case GLFW_KEY_V:
            _simulation.vortex(xpos, ypos, 200.0f, 60.0f, 50.0f);
            break;
        case GLFW_KEY_S:
            break;
        default:
            break;
        }
    }
}

Application::~Application()
{
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void Application::run()
{
    while (!glfwWindowShouldClose(_window))
    {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        glClear(GL_COLOR_BUFFER_BIT);

        _renderer.setupProjection(width, height);
        _renderer.drawCanvas(_simulation);

        if (_isDragging)
        {
            double currentX, currentY;
            glfwGetCursorPos(_window, &currentX, &currentY);
            _renderer.drawDragLine(Point(_startX, _startY), Point(currentX, currentY));
        }

        glfwSwapBuffers(_window);
        glfwPollEvents();
    }
}