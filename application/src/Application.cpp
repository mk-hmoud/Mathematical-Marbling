#include "Application.h"

#include <iostream>
#include <ctime>
#include <cstdlib>

Application::Application(int width, int height, const char *title)
    : _simulation(width, height), _tineMode(false), _isDragging(false), _startX(0), _startY(0)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 8);
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window)
    {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(_window);
    glfwSetWindowUserPointer(_window, this);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(_window);
        glfwTerminate();
        return;
    }

    glEnable(GL_MULTISAMPLE);

    glfwSetMouseButtonCallback(_window, [](GLFWwindow *win, int button, int action, int mods)
                               {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(win));
        if (app) app->handleMouseInput(button, action, mods); });

    glfwSetKeyCallback(_window, [](GLFWwindow *win, int key, int scancode, int action, int mods)
                       {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(win));
        if (app) app->handleKeyInput(key, scancode, action, mods); });
}

void Application::handleMouseInput(int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

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
                _simulation.tine(Point(static_cast<float>(_startX), static_cast<float>(_startY)),
                                 Point(static_cast<float>(xpos), static_cast<float>(ypos)), 80.0f, 16.0f);
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

void Application::handleKeyInput(int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;

    double mx, my;
    glfwGetCursorPos(_window, &mx, &my);
    float xpos = static_cast<float>(mx);
    float ypos = static_cast<float>(my);

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
        // TODO: Implement save
        break;
    default:
        break;
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

        std::vector<Polygon> polygons = _simulation.getPolygons();
        _renderer.drawPolygons(polygons);

        if (_isDragging)
        {
            double cx, cy;
            glfwGetCursorPos(_window, &cx, &cy);
            _renderer.drawDragLine(Point(static_cast<float>(_startX), static_cast<float>(_startY)),
                                   Point(static_cast<float>(cx), static_cast<float>(cy)));
        }

        glfwSwapBuffers(_window);
        glfwPollEvents();
    }
}