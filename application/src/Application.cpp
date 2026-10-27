#include "Application.h"
#include <iostream>
#include <memory>

#include "Renderer.h"
#include "PngWriter.h"

Application::Application(int width, int height, const char *title)
    : _window(nullptr), _artwork(width, height), _isDragging(false), _startX(0), _startY(0)
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
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK && glewStatus != GLEW_ERROR_NO_GLX_DISPLAY)
    {
        std::cerr << "Failed to initialize GLEW: "
                  << glewGetErrorString(glewStatus) << std::endl;
        glfwDestroyWindow(_window);
        _window = nullptr;
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
    if (_window)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        _renderer.reset();

        glfwDestroyWindow(_window);
    }
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
    Point endPos(static_cast<float>(xpos), static_cast<float>(ypos));

    if (action == GLFW_PRESS)
    {
        _isDragging = true;
        _startX = xpos;
        _startY = ypos;

        if (_toolMode == ToolMode::Drop)
        {
            _artwork.addDrop(endPos, _dropRadius, _nextDropColor);
        }
        else if (_toolMode == ToolMode::Vortex)
        {
            _artwork.addVortex(endPos, _vortexSettings);
        }
    }
    else if (action == GLFW_RELEASE)
    {
        if (_isDragging)
        {
            Point startPos(static_cast<float>(_startX), static_cast<float>(_startY));

            switch (_toolMode)
            {
            case ToolMode::Tine:
                _artwork.addTine(startPos, endPos, _tineSettings);
                break;
            case ToolMode::Comb:
                _artwork.addComb(startPos, endPos, _combSettings);
                break;
            case ToolMode::Drop:
            case ToolMode::Vortex:
                break;
            }
            _isDragging = false;
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
    Point pos(static_cast<float>(mx), static_cast<float>(my));

    switch (key)
    {
    case GLFW_KEY_D:
        _toolMode = ToolMode::Drop;
        std::cout << "Tool: Ink Drop" << std::endl;
        break;
    case GLFW_KEY_T:
        _toolMode = ToolMode::Tine;
        std::cout << "Tool: Single Tine" << std::endl;
        break;
    case GLFW_KEY_C:
        _toolMode = ToolMode::Comb;
        std::cout << "Tool: Comb" << std::endl;
        break;
    case GLFW_KEY_BACKSPACE:
        _artwork.clear();
        break;
    case GLFW_KEY_V:
        _artwork.addVortex(pos, _vortexSettings);
        break;
    }
}

void Application::addRandomDrop()
{
    _artwork.addRandomDrop(_dropRadius);
}

void Application::exportPng()
{
    const float scale = _exportScale < 0.1f ? 0.1f : _exportScale;
    const int pw = static_cast<int>(_artwork.width() * scale);
    const int ph = static_cast<int>(_artwork.height() * scale);

    std::vector<unsigned char> pixels = _renderer->renderToPixels(
        _artwork.getPolygons(), pw, ph,
        static_cast<float>(_artwork.width()), static_cast<float>(_artwork.height()),
        _backgroundColor);

    if (pixels.empty())
    {
        _exportStatus = "Export failed: could not render offscreen image.";
        return;
    }

    if (writePng(_exportPath, pw, ph, pixels.data()))
        _exportStatus = std::string("Saved ") + _exportPath + " (" +
                        std::to_string(pw) + "x" + std::to_string(ph) + ")";
    else
        _exportStatus = std::string("Export failed: could not write ") + _exportPath;

    std::cout << _exportStatus << std::endl;
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
        glClearColor(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);

        _renderer->setupProjection(width, height);

        // Re-tessellate and re-upload only when the ink geometry changed.
        if (_artwork.isDirty())
        {
            _renderer->updateBatch(_artwork.getPolygons());
            _artwork.clearDirty();
        }
        _renderer->drawBatch();

        if (_isDragging)
        {
            double currentMouseX, currentMouseY;
            glfwGetCursorPos(_window, &currentMouseX, &currentMouseY);
            Point dragStart(static_cast<float>(_startX), static_cast<float>(_startY));
            Point dragEnd(static_cast<float>(currentMouseX), static_cast<float>(currentMouseY));

            if (_toolMode == ToolMode::Tine)
            {
                // Draw a single drag line for the tine
                _renderer->drawDragLine(dragStart, dragEnd);
            }
            else if (_toolMode == ToolMode::Comb)
            {
                std::vector<std::pair<Point, Point>> combLines;

                Point direction = dragEnd - dragStart;
                float length = direction.length();
                if (length > 1e-6f) // Avoid division by zero
                {
                    direction.normalize();

                    // Calculate the perpendicular vector for spacing
                    Point perpendicular(-direction.y, direction.x);

                    // Determine the "center" of the comb handle (midpoint of the drag line)
                    Point combHandleCenter = dragStart + direction * (length / 2.0f);

                    // Total width of the comb teeth
                    float totalCombWidth = _combSettings.spacing * (static_cast<float>(_combSettings.numTines) - 1.0f);

                    // Starting offset to center the comb
                    Point offset = perpendicular * (totalCombWidth / 2.0f);

                    for (int i = 0; i < _combSettings.numTines; ++i)
                    {
                        Point tineStart = combHandleCenter - offset + perpendicular * (static_cast<float>(i) * _combSettings.spacing);
                        Point tineEnd = tineStart + direction * length; // Each tine has the same length as the drag

                        combLines.push_back({tineStart, tineEnd});
                    }
                }
                _renderer->drawCombLines(combLines, {1.0f, 1.0f, 1.0f}); // White comb lines
            }
        }

        // --- MENU BAR ---
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Export PNG"))
                    exportPng();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Drop", "D", _toolMode == ToolMode::Drop))
                    _toolMode = ToolMode::Drop;
                if (ImGui::MenuItem("Tine", "T", _toolMode == ToolMode::Tine))
                    _toolMode = ToolMode::Tine;
                if (ImGui::MenuItem("Comb", "C", _toolMode == ToolMode::Comb))
                    _toolMode = ToolMode::Comb;
                if (ImGui::MenuItem("Vortex", "V", _toolMode == ToolMode::Vortex))
                    _toolMode = ToolMode::Vortex;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Actions"))
            {
                if (ImGui::MenuItem("Add Random Ink Drop"))
                    addRandomDrop();
                ImGui::Separator();
                if (ImGui::MenuItem("Clear Canvas", "Backspace"))
                    _artwork.clear();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // --- GUI DRAWING ---
        {
            ImGui::Begin("Marbling Controls");

            const char *toolNames[] = {"Drop", "Tine", "Comb", "Vortex"};
            ImGui::Text("Active Tool: %s", toolNames[static_cast<int>(_toolMode)]);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Drop Settings"))
            {
                ImGui::SliderFloat("Drop Radius", &_dropRadius, 10.0f, 150.0f);
                ImGui::ColorEdit3("Drop Color", (float *)&_nextDropColor);
            }

            if (ImGui::CollapsingHeader("Tine Settings"))
            {
                ImGui::SliderFloat("Tine Strength (z)", &_tineSettings.z, 10.0f, 200.0f);
                ImGui::SliderFloat("Tine Falloff (c)", &_tineSettings.c, 1.0f, 50.0f);
            }

            if (ImGui::CollapsingHeader("Comb Settings (New!)"))
            {
                ImGui::SliderInt("Num Tines", &_combSettings.numTines, 2, 50);
                ImGui::SliderFloat("Tine Spacing", &_combSettings.spacing, 5.0f, 100.0f);
                ImGui::Separator();
                ImGui::SliderFloat("Comb Strength (z)", &_combSettings.properties.z, 10.0f, 200.0f);
                ImGui::SliderFloat("Comb Falloff (c)", &_combSettings.properties.c, 1.0f, 50.0f);
            }

            if (ImGui::CollapsingHeader("Vortex Settings"))
            {
                ImGui::SliderFloat("Vortex Strength (z)", &_vortexSettings.z, 50.0f, 500.0f);
                ImGui::SliderFloat("Vortex C-Value (c)", &_vortexSettings.c, 10.0f, 100.0f);
                ImGui::SliderFloat("Vortex Radius (r)", &_vortexSettings.r, 10.0f, 100.0f);
            }

            if (ImGui::CollapsingHeader("Canvas", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3("Background Color", (float *)&_backgroundColor);
            }

            if (ImGui::CollapsingHeader("Export", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("File", _exportPath, sizeof(_exportPath));
                ImGui::SliderFloat("Scale", &_exportScale, 0.5f, 8.0f, "%.1fx");
                ImGui::Text("Output: %d x %d px",
                            static_cast<int>(_artwork.width() * _exportScale),
                            static_cast<int>(_artwork.height() * _exportScale));
                if (ImGui::Button("Export PNG"))
                {
                    exportPng();
                }
                if (!_exportStatus.empty())
                    ImGui::TextWrapped("%s", _exportStatus.c_str());
            }

            ImGui::Separator();
            if (ImGui::Button("Add Random Ink Drop"))
            {
                addRandomDrop();
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