#include "Application.h"
#include <iostream>
#include <memory>
#include <algorithm>

#include "Renderer.h"
#include "PngWriter.h"
#include "MarbleIO.h"

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

    glfwSetScrollCallback(_window, [](GLFWwindow *win, double xoffset, double yoffset)
                          {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(win));
        if (app) app->handleScroll(xoffset, yoffset); });

    // Cap zoom so the canvas viewport never exceeds the driver's limit.
    GLint viewportDims[2] = {8192, 8192};
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportDims);
    _maxViewportDim = std::min(viewportDims[0], viewportDims[1]);

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

Application::CanvasView Application::canvasView(int fbWidth, int fbHeight) const
{
    const float cw = static_cast<float>(_artwork.width());
    const float ch = static_cast<float>(_artwork.height());

    // Fit the canvas inside the window, preserving aspect (letterbox), then
    // apply the current zoom and pan.
    float fit = std::min(fbWidth / cw, fbHeight / ch);
    if (fit <= 0.0f)
        fit = 1.0f;
    const float scale = fit * _zoom;

    const float drawW = cw * scale;
    const float drawH = ch * scale;
    const float offX = (fbWidth - drawW) * 0.5f + _panX;
    const float offY = (fbHeight - drawH) * 0.5f + _panY; // from the top

    CanvasView v;
    v.scale = scale;
    v.offX = offX;
    v.offY = offY;
    v.vpW = static_cast<int>(drawW);
    v.vpH = static_cast<int>(drawH);
    v.vpX = static_cast<int>(offX);
    v.vpY = static_cast<int>(fbHeight - offY - drawH); // GL origin is bottom-left
    return v;
}

Point Application::windowToCanvas(double windowX, double windowY) const
{
    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    glfwGetWindowSize(_window, &winW, &winH);
    if (winW == 0 || winH == 0)
        return Point(0.0f, 0.0f);

    // Cursor is in window coords; convert to framebuffer px (HiDPI-safe).
    const double fx = windowX * static_cast<double>(fbW) / winW;
    const double fy = windowY * static_cast<double>(fbH) / winH;

    const CanvasView v = canvasView(fbW, fbH);
    return Point(static_cast<float>((fx - v.offX) / v.scale),
                 static_cast<float>((fy - v.offY) / v.scale));
}

void Application::handleScroll(double xoffset, double yoffset)
{
    (void)xoffset;
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse || yoffset == 0.0)
        return;

    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    glfwGetWindowSize(_window, &winW, &winH);
    if (winW == 0 || winH == 0)
        return;

    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(_window, &mx, &my);
    const Point anchor = windowToCanvas(mx, my); // canvas point under cursor

    // Clamp zoom to a sane range and to what keeps the viewport within limits.
    const float cw = static_cast<float>(_artwork.width());
    const float ch = static_cast<float>(_artwork.height());
    const float fit = std::min(fbW / cw, fbH / ch);
    const float zoomCap = std::min(_maxViewportDim / (cw * fit), _maxViewportDim / (ch * fit));

    const float factor = (yoffset > 0.0) ? 1.1f : 1.0f / 1.1f;
    float newZoom = std::clamp(_zoom * factor, 0.2f, std::min(20.0f, zoomCap));
    if (newZoom == _zoom)
        return;
    _zoom = newZoom;

    // Re-pan so the anchored canvas point stays under the cursor.
    const double fx = mx * static_cast<double>(fbW) / winW;
    const double fy = my * static_cast<double>(fbH) / winH;
    const float scale = fit * _zoom;
    const float drawW = cw * scale;
    const float drawH = ch * scale;
    _panX = static_cast<float>(fx) - anchor.x * scale - (fbW - drawW) * 0.5f;
    _panY = static_cast<float>(fy) - anchor.y * scale - (fbH - drawH) * 0.5f;
}

void Application::resetView()
{
    _zoom = 1.0f;
    _panX = 0.0f;
    _panY = 0.0f;
}

void Application::handleMouseInput(int button, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    // Middle-button drag pans the view.
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if (action == GLFW_PRESS)
        {
            _isPanning = true;
            glfwGetCursorPos(_window, &_panLastX, &_panLastY);
        }
        else if (action == GLFW_RELEASE)
        {
            _isPanning = false;
        }
        return;
    }

    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    Point endPos = windowToCanvas(xpos, ypos); // canvas coordinates

    if (action == GLFW_PRESS)
    {
        _isDragging = true;
        _startX = endPos.x;
        _startY = endPos.y;

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
            _artwork.endPreview(); // committed op now shows from the real canvas
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
    Point pos = windowToCanvas(mx, my); // canvas coordinates

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
    case GLFW_KEY_Z:
        if (mods & GLFW_MOD_CONTROL)
        {
            if (mods & GLFW_MOD_SHIFT)
                _artwork.redo();
            else
                _artwork.undo();
        }
        break;
    case GLFW_KEY_Y:
        if (mods & GLFW_MOD_CONTROL)
            _artwork.redo();
        break;
    case GLFW_KEY_0:
        resetView();
        break;
    }
}

void Application::addRandomDrop()
{
    if (_randomUsesPalette && !_palettes.empty())
        _artwork.addRandomDrop(_dropRadius, _palettes[_activePalette].colors);
    else
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

void Application::saveProject()
{
    if (MarbleIO::save(_projectPath, _artwork.width(), _artwork.height(), _artwork.operations()))
        _projectStatus = std::string("Saved ") + _projectPath + " (" +
                         std::to_string(_artwork.operations().size()) + " ops)";
    else
        _projectStatus = std::string("Save failed: could not write ") + _projectPath;

    std::cout << _projectStatus << std::endl;
}

void Application::loadProject()
{
    int width = 0, height = 0;
    std::vector<Operation> ops;
    if (MarbleIO::load(_projectPath, width, height, ops))
    {
        _artwork.load(width, height, ops);
        _projectStatus = std::string("Loaded ") + _projectPath + " (" +
                         std::to_string(ops.size()) + " ops)";
    }
    else
    {
        _projectStatus = std::string("Load failed: could not read ") + _projectPath;
    }

    std::cout << _projectStatus << std::endl;
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

        // Apply any middle-button pan (delta in framebuffer px).
        if (_isPanning)
        {
            int winW = 0, winH = 0;
            glfwGetWindowSize(_window, &winW, &winH);
            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(_window, &mx, &my);
            if (winW > 0 && winH > 0)
            {
                _panX += static_cast<float>((mx - _panLastX) * width / winW);
                _panY += static_cast<float>((my - _panLastY) * height / winH);
            }
            _panLastX = mx;
            _panLastY = my;
        }

        // Clear the whole window to the "outside" color.
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, width, height);
        glClearColor(_outsideColor.r, _outsideColor.g, _outsideColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Confine drawing to the canvas rectangle and clear it to the canvas
        // background, so the canvas reads as a framed panel in the window.
        const CanvasView view = canvasView(width, height);
        glEnable(GL_SCISSOR_TEST);
        glScissor(view.vpX, view.vpY, view.vpW, view.vpH);
        glClearColor(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        _renderer->setView(view.vpX, view.vpY, view.vpW, view.vpH,
                           static_cast<float>(_artwork.width()),
                           static_cast<float>(_artwork.height()));

        // Live preview: deform the ink under the cursor without recording, so
        // the batch below picks up the in-progress gesture this same frame.
        Point dragStart(static_cast<float>(_startX), static_cast<float>(_startY));
        Point dragEnd = dragStart;
        if (_isDragging)
        {
            double currentMouseX, currentMouseY;
            glfwGetCursorPos(_window, &currentMouseX, &currentMouseY);
            dragEnd = windowToCanvas(currentMouseX, currentMouseY);

            if (_toolMode == ToolMode::Tine)
                _artwork.previewTine(dragStart, dragEnd, _tineSettings);
            else if (_toolMode == ToolMode::Comb)
                _artwork.previewComb(dragStart, dragEnd, _combSettings);
        }

        // Re-tessellate and re-upload only when the ink geometry changed.
        if (_artwork.isDirty())
        {
            _renderer->updateBatch(_artwork.getPolygons());
            _artwork.clearDirty();
        }
        _renderer->drawBatch();

        if (_isDragging)
        {
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

        // Done with the canvas rectangle; let ImGui draw over the full window.
        glDisable(GL_SCISSOR_TEST);

        // --- MENU BAR ---
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save"))
                    saveProject();
                if (ImGui::MenuItem("Load"))
                    loadProject();
                ImGui::Separator();
                if (ImGui::MenuItem("Export PNG"))
                    exportPng();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, _artwork.canUndo()))
                    _artwork.undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, _artwork.canRedo()))
                    _artwork.redo();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Reset View", "0"))
                    resetView();
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

            if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (!_palettes.empty())
                {
                    if (ImGui::BeginCombo("Set", _palettes[_activePalette].name.c_str()))
                    {
                        for (int i = 0; i < static_cast<int>(_palettes.size()); ++i)
                        {
                            const bool selected = (i == _activePalette);
                            if (ImGui::Selectable(_palettes[i].name.c_str(), selected))
                                _activePalette = i;
                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    // Clickable swatches that set the active drop color. They
                    // wrap to the window width.
                    const std::vector<Color> &colors = _palettes[_activePalette].colors;
                    const float swatch = 28.0f;
                    const float visibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    const ImGuiStyle &style = ImGui::GetStyle();
                    for (int i = 0; i < static_cast<int>(colors.size()); ++i)
                    {
                        ImGui::PushID(i);
                        const ImVec4 col(colors[i].r, colors[i].g, colors[i].b, 1.0f);
                        if (ImGui::ColorButton("##swatch", col, ImGuiColorEditFlags_NoTooltip,
                                               ImVec2(swatch, swatch)))
                        {
                            _nextDropColor = colors[i];
                        }
                        ImGui::PopID();

                        const float nextX = ImGui::GetItemRectMax().x + style.ItemSpacing.x + swatch;
                        if (i + 1 < static_cast<int>(colors.size()) && nextX < visibleX)
                            ImGui::SameLine();
                    }
                }

                ImGui::ColorEdit3("Drop Color", (float *)&_nextDropColor);
                ImGui::Checkbox("Random drops use palette", &_randomUsesPalette);
            }

            if (ImGui::CollapsingHeader("Drop Settings"))
            {
                ImGui::SliderFloat("Drop Radius", &_dropRadius, 10.0f, 150.0f);
                ImGui::ColorEdit3("Drop Color##settings", (float *)&_nextDropColor);
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
                ImGui::Text("Current: %d x %d", _artwork.width(), _artwork.height());
                ImGui::Text("Zoom: %.0f%%  (scroll to zoom, middle-drag to pan)", _zoom * 100.0f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset View"))
                    resetView();

                ImGui::InputInt2("Size", _canvasSize);

                // Aspect-ratio presets (long edge ~1200).
                struct Preset { const char *label; int w, h; };
                static const Preset presets[] = {
                    {"1:1", 1000, 1000}, {"4:3", 1200, 900}, {"3:2", 1200, 800},
                    {"16:9", 1280, 720}, {"3:4", 900, 1200}};
                for (int i = 0; i < 5; ++i)
                {
                    if (i > 0)
                        ImGui::SameLine();
                    if (ImGui::SmallButton(presets[i].label))
                    {
                        _canvasSize[0] = presets[i].w;
                        _canvasSize[1] = presets[i].h;
                    }
                }

                if (ImGui::Button("New Canvas"))
                {
                    const int w = std::clamp(_canvasSize[0], 16, 8192);
                    const int h = std::clamp(_canvasSize[1], 16, 8192);
                    _canvasSize[0] = w;
                    _canvasSize[1] = h;
                    _artwork.load(w, h, {}); // fresh canvas at the chosen size
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(clears the current piece)");
            }

            if (ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("Path", _projectPath, sizeof(_projectPath));
                if (ImGui::Button("Save"))
                    saveProject();
                ImGui::SameLine();
                if (ImGui::Button("Load"))
                    loadProject();
                if (!_projectStatus.empty())
                    ImGui::TextWrapped("%s", _projectStatus.c_str());
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