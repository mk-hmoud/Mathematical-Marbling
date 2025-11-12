#include "Application.h"
#include <iostream>
#include <memory>

#include "Renderer.h"

Application::Application(int width, int height, const char *title)
    : _simulation(width, height), _tineMode(false), _isDragging(false), _startX(0), _startY(0)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_SAMPLES, 8);
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(_window);
    glfwSetWindowUserPointer(_window, this);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(_window);
        glfwTerminate();
        return;
    }

    _renderer = std::make_unique<Renderer>();

    glfwSetMouseButtonCallback(_window, [](GLFWwindow *win, int button, int action, int mods)
                               {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(win));
        if (app) app->handleMouseInput(button, action, mods); });

    glfwSetKeyCallback(_window, [](GLFWwindow *win, int key, int scancode, int action, int mods)
                       {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(win));
        if (app) app->handleKeyInput(key, scancode, action, mods); });

    // --- IMGUI INIT ---
    const char *glsl_version = "#version 330";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

Application::~Application()
{
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void Application::handleMouseInput(int button, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

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
            _simulation.add_drop(static_cast<float>(xpos), static_cast<float>(ypos), _dropRadius, _nextDropColor);
        }
    }
}

void Application::handleKeyInput(int key, int scancode, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return;

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
        _simulation.vortex(xpos, ypos, _vortexStrength, 60.0f, _vortexRadius);
        break;
    case GLFW_KEY_S:
        // TODO: Implement save
        break;
    default:
        break;
    }
}

void Application::run()
{
    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);

        _renderer->setupProjection(width, height);

        std::vector<Polygon> polygons = _simulation.getPolygons();
        _renderer->drawPolygons(polygons);

        if (_isDragging)
        {
            double cx, cy;
            glfwGetCursorPos(_window, &cx, &cy);
            _renderer->drawDragLine(Point(static_cast<float>(_startX), static_cast<float>(_startY)),
                                    Point(static_cast<float>(cx), static_cast<float>(cy)));
        }

        // --- GUI DRAWING ---
        {
            ImGui::Begin("Marbling Controls");

            ImGui::Checkbox("Tine Mode (T)", &_tineMode);
            ImGui::Separator();
            ImGui::Text("Vortex (V)");
            ImGui::SliderFloat("Strength", &_vortexStrength, 50.0f, 500.0f);
            ImGui::SliderFloat("Radius", &_vortexRadius, 10.0f, 100.0f);
            ImGui::Separator();
            ImGui::Text("Next Ink Drop");
            ImGui::SliderFloat("Drop Radius", &_dropRadius, 10.0f, 150.0f);
            ImGui::ColorEdit3("Color", (float *)&_nextDropColor);
            ImGui::Separator();
            if (ImGui::Button("Clear Canvas (Backspace)"))
            {
                _simulation.clear();
            }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // --- IMGUI RENDER ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Handle ImGui Viewports
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(_window);
    }
}