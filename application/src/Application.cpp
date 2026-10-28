#include "Application.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <cfloat>

#include "imgui_internal.h" // DockBuilder*, DockNode, BeginViewportSideBar

#include "Renderer.h"
#include "PngWriter.h"
#include "SvgWriter.h"
#include "MarbleIO.h"
#include "FileDialog.h"

namespace
{
    void copyToBuffer(char *buf, size_t n, const std::string &s)
    {
        std::snprintf(buf, n, "%s", s.c_str());
    }

    // "(?)" tooltip marker for inline help.
    void helpMarker(const char *desc)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

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
    glfwWindowHint(GLFW_STENCIL_BITS, 8); // needed for radial (wedge) symmetry
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
    // Viewports (tear-off OS windows) are intentionally left off: a single
    // cohesive window reads more like a finished tool and sidesteps Wayland's
    // inability to position child windows.

    // HiDPI: bake fonts larger and scale metrics so the UI stays crisp.
    float sx = 1.0f, sy = 1.0f;
    glfwGetWindowContentScale(_window, &sx, &sy);
    _dpiScale = std::max({sx, sy, 1.0f});

    _fonts = Theme::LoadFonts(io, _dpiScale);
    Theme::Apply(_dpiScale);

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

    // Canvas lives inside the dockspace central node (region). Fall back to the
    // whole framebuffer until the region has been measured.
    const bool haveRegion = _regW > 1.0f && _regH > 1.0f;
    const float rx = haveRegion ? _regX : 0.0f;
    const float ry = haveRegion ? _regY : 0.0f;
    const float rw = haveRegion ? _regW : static_cast<float>(fbWidth);
    const float rh = haveRegion ? _regH : static_cast<float>(fbHeight);

    // Fit the canvas inside the region, preserving aspect (letterbox), then
    // apply the current zoom and pan.
    float fit = std::min(rw / cw, rh / ch);
    if (fit <= 0.0f)
        fit = 1.0f;
    const float scale = fit * _zoom;

    const float drawW = cw * scale;
    const float drawH = ch * scale;
    const float offX = rx + (rw - drawW) * 0.5f + _panX;        // from the left
    const float offY = ry + (rh - drawH) * 0.5f + _panY;        // from the top

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
    const bool haveRegion = _regW > 1.0f && _regH > 1.0f;
    const float rx = haveRegion ? _regX : 0.0f;
    const float ry = haveRegion ? _regY : 0.0f;
    const float rw = haveRegion ? _regW : static_cast<float>(fbW);
    const float rh = haveRegion ? _regH : static_cast<float>(fbH);
    const float fit = std::min(rw / cw, rh / ch);
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
    _panX = static_cast<float>(fx) - anchor.x * scale - (rx + (rw - drawW) * 0.5f);
    _panY = static_cast<float>(fy) - anchor.y * scale - (ry + (rh - drawH) * 0.5f);
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
            placeDrop(endPos);
        }
        else if (_toolMode == ToolMode::Vortex)
        {
            _artwork.addVortex(endPos, _vortexSettings);
        }
        else if (_toolMode == ToolMode::Eyedropper)
        {
            pickColor(xpos, ypos);
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
            case ToolMode::Wave:
                _artwork.addWave(startPos, endPos, _waveSettings);
                break;
            case ToolMode::Drop:
            case ToolMode::Vortex:
            case ToolMode::Eyedropper:
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
    case GLFW_KEY_E:
        _toolMode = ToolMode::Eyedropper;
        std::cout << "Tool: Eyedropper" << std::endl;
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
    case GLFW_KEY_W:
        _toolMode = ToolMode::Wave;
        std::cout << "Tool: Wave" << std::endl;
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

void Application::placeDrop(const Point &center)
{
    if (!_concentricDrop)
    {
        _artwork.addDrop(center, _dropRadius, _nextDropColor);
        return;
    }

    // Concentric rings: drop decreasing radii at the same point, each inner
    // drop pushing the previous outward into a ring. Colors cycle through the
    // active palette (falling back to the current drop color).
    const std::vector<Color> &pal =
        (!_palettes.empty()) ? _palettes[_activePalette].colors : std::vector<Color>{};

    _artwork.beginBatch();
    for (int i = 0; i < _ringCount; ++i)
    {
        const float r = _dropRadius * (1.0f - static_cast<float>(i) / _ringCount);
        const Color c = pal.empty() ? _nextDropColor : pal[i % pal.size()];
        _artwork.addDrop(center, r, c);
    }
    _artwork.endBatch();
}

void Application::pickColor(double windowX, double windowY)
{
    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    glfwGetWindowSize(_window, &winW, &winH);
    if (winW == 0 || winH == 0)
        return;

    // Read the rendered pixel under the cursor (GL origin is bottom-left).
    const int px = static_cast<int>(windowX * fbW / winW);
    const int py = static_cast<int>(fbH - windowY * fbH / winH);
    unsigned char rgba[4] = {0, 0, 0, 0};
    glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    _nextDropColor = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f};
}

void Application::addRandomDrop()
{
    if (_randomUsesPalette && !_palettes.empty())
        _artwork.addRandomDrop(_dropRadius, _palettes[_activePalette].colors);
    else
        _artwork.addRandomDrop(_dropRadius);
}

namespace
{
    // A comb spanning `span` perpendicular to its pull, with the given tooth
    // spacing and shared tine properties.
    CombSettings spanningComb(float span, float spacing, const TineSettings &props)
    {
        CombSettings c;
        c.spacing = spacing;
        c.numTines = std::max(2, static_cast<int>(span / spacing));
        c.properties = props;
        return c;
    }
}

void Application::applyStone()
{
    // Scatter a field of displacing drops: the classic marbled "stone" base.
    const float radius = std::min(_artwork.width(), _artwork.height()) / 8.0f;
    const std::vector<Color> palette =
        (!_palettes.empty()) ? _palettes[_activePalette].colors : std::vector<Color>{};

    _artwork.beginBatch();
    for (int i = 0; i < _stoneDrops; ++i)
        _artwork.addRandomDrop(radius, palette);
    _artwork.endBatch();
}

void Application::applyNonpareil()
{
    const float cw = static_cast<float>(_artwork.width());
    const float ch = static_cast<float>(_artwork.height());

    _artwork.beginBatch();
    // Fine comb pulled down (teeth across the width), then across (teeth down).
    _artwork.addComb(Point(cw * 0.5f, 0.0f), Point(cw * 0.5f, ch),
                     spanningComb(cw, _patternSpacing, _combSettings.properties));
    _artwork.addComb(Point(0.0f, ch * 0.5f), Point(cw, ch * 0.5f),
                     spanningComb(ch, _patternSpacing, _combSettings.properties));
    _artwork.endBatch();
}

void Application::applyGelGit()
{
    const float cw = static_cast<float>(_artwork.width());
    const float ch = static_cast<float>(_artwork.height());

    _artwork.beginBatch();
    // Back-and-forth vertical passes, each offset half a tooth, which shears
    // the ink into interlocking waves.
    for (int p = 0; p < 3; ++p)
    {
        const float x = cw * 0.5f + (p % 2) * _patternSpacing * 0.5f;
        const CombSettings c = spanningComb(cw, _patternSpacing, _combSettings.properties);
        if (p % 2 == 0)
            _artwork.addComb(Point(x, 0.0f), Point(x, ch), c); // pull down
        else
            _artwork.addComb(Point(x, ch), Point(x, 0.0f), c); // pull up
    }
    _artwork.endBatch();
}

void Application::applyChevron()
{
    const float cw = static_cast<float>(_artwork.width());
    const float ch = static_cast<float>(_artwork.height());
    const float diag = std::sqrt(cw * cw + ch * ch);

    _artwork.beginBatch();
    // A vertical comb, then a diagonal comb crossing it, forming chevrons.
    _artwork.addComb(Point(cw * 0.5f, 0.0f), Point(cw * 0.5f, ch),
                     spanningComb(cw, _patternSpacing, _combSettings.properties));
    _artwork.addComb(Point(0.0f, 0.0f), Point(cw, ch),
                     spanningComb(diag, _patternSpacing, _combSettings.properties));
    _artwork.endBatch();
}

void Application::exportPng()
{
    const float scale = _exportScale < 0.1f ? 0.1f : _exportScale;
    const int pw = static_cast<int>(_artwork.width() * scale);
    const int ph = static_cast<int>(_artwork.height() * scale);

    std::vector<unsigned char> pixels = _renderer->renderToPixels(
        _artwork.getPolygons(), pw, ph,
        static_cast<float>(_artwork.width()), static_cast<float>(_artwork.height()),
        _backgroundColor, _symmetry, _exportTransparent, _symmetryFold);

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

void Application::exportSvg()
{
    if (writeSvg(_svgPath, _artwork.width(), _artwork.height(),
                 _artwork.getPolygons(), _backgroundColor, _exportTransparent))
        _exportStatus = std::string("Saved ") + _svgPath + " (vector)";
    else
        _exportStatus = std::string("Export failed: could not write ") + _svgPath;

    std::cout << _exportStatus << std::endl;
}

void Application::promptSaveProject()
{
    if (FileDialog::available())
    {
        const std::string p = FileDialog::save("Save Marbling Project", _projectPath);
        if (p.empty())
            return; // cancelled
        copyToBuffer(_projectPath, sizeof(_projectPath), p);
    }
    saveProject();
}

void Application::promptLoadProject()
{
    if (FileDialog::available())
    {
        const std::string p = FileDialog::open("Open Marbling Project");
        if (p.empty())
            return;
        copyToBuffer(_projectPath, sizeof(_projectPath), p);
    }
    loadProject();
}

void Application::promptExportPng()
{
    if (FileDialog::available())
    {
        const std::string p = FileDialog::save("Export PNG", _exportPath);
        if (p.empty())
            return;
        copyToBuffer(_exportPath, sizeof(_exportPath), p);
    }
    exportPng();
}

void Application::promptExportSvg()
{
    if (FileDialog::available())
    {
        const std::string p = FileDialog::save("Export SVG", _svgPath);
        if (p.empty())
            return;
        copyToBuffer(_svgPath, sizeof(_svgPath), p);
    }
    exportSvg();
}

void Application::exportTimelapse()
{
    const std::vector<Operation> &ops = _artwork.operations();
    if (ops.empty())
    {
        _exportStatus = "Timelapse: nothing to replay.";
        return;
    }

    const int frames = std::clamp(_timelapseFrames, 2, 600);
    const int w = _artwork.width();
    const int h = _artwork.height();
    const float scale = _exportScale < 0.1f ? 0.1f : _exportScale;
    const int pw = static_cast<int>(w * scale);
    const int ph = static_cast<int>(h * scale);

    std::error_code ec;
    std::filesystem::create_directories(_timelapseDir, ec);

    // Replay growing prefixes of the history into a temp artwork, mirroring the
    // current recolor so frames match the live look.
    Artwork frame(w, h);
    if (_artwork.isRecolorActive())
        frame.setRecolor(_artwork.recolorPalette());

    int written = 0;
    for (int f = 1; f <= frames; ++f)
    {
        size_t k = static_cast<size_t>(
            static_cast<double>(f) / frames * ops.size() + 0.5);
        k = std::clamp<size_t>(k, 1, ops.size());

        frame.setOperations(std::vector<Operation>(ops.begin(), ops.begin() + k));

        std::vector<unsigned char> px = _renderer->renderToPixels(
            frame.getPolygons(), pw, ph, static_cast<float>(w), static_cast<float>(h),
            _backgroundColor, _symmetry, _exportTransparent, _symmetryFold);
        if (px.empty())
        {
            _exportStatus = "Timelapse failed: could not render a frame.";
            return;
        }

        char name[512];
        std::snprintf(name, sizeof(name), "%s/frame_%04d.png", _timelapseDir, f);
        if (!writePng(name, pw, ph, px.data()))
        {
            _exportStatus = std::string("Timelapse failed: could not write ") + name;
            return;
        }
        ++written;
    }

    // The final frame replayed the full history, so the renderer's batch still
    // matches the live artwork; no re-upload needed.
    _exportStatus = "Wrote " + std::to_string(written) + " frames to '" +
                    _timelapseDir + "'. Assemble with: ffmpeg -r 30 -i " +
                    _timelapseDir + "/frame_%04d.png timelapse.mp4";
    std::cout << _exportStatus << std::endl;
}

void Application::saveProject()
{
    const std::vector<Color> recolor =
        _artwork.isRecolorActive() ? _artwork.recolorPalette() : std::vector<Color>{};
    if (MarbleIO::save(_projectPath, _artwork.width(), _artwork.height(),
                       _artwork.operations(), recolor))
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
    std::vector<Color> recolor;
    if (MarbleIO::load(_projectPath, width, height, ops, recolor))
    {
        _artwork.load(width, height, ops);
        if (!recolor.empty())
            _artwork.setRecolor(recolor);
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

        // --- UI (built before the canvas so it can target the central node) ---
        // Menu bar and status bar first: they reserve work-area space that the
        // dock host below reads when sizing the central (canvas) node.
        drawMenuBar();
        drawStatusBar();

        // Full-viewport dock host with a transparent (passthru) central node:
        // the GL canvas shows through the hole, panels dock around the sides.
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        const ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("##DockHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        const ImGuiID dockId = ImGui::GetID("MarblingDockSpace");
        static bool dockInitialized = false;
        if (!dockInitialized)
        {
            dockInitialized = true;
            // Only impose our layout when there is no saved one (imgui.ini).
            if (ImGui::DockBuilderGetNode(dockId) == nullptr)
                buildDefaultDockLayout(dockId);
        }
        ImGui::DockSpace(dockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();

        // Measure the central node -> canvas region (framebuffer px, top-left).
        if (ImGuiDockNode *central = ImGui::DockBuilderGetCentralNode(dockId))
        {
            int winW = 0, winH = 0;
            glfwGetWindowSize(_window, &winW, &winH);
            const float sx = winW > 0 ? static_cast<float>(width) / winW : 1.0f;
            const float sy = winH > 0 ? static_cast<float>(height) / winH : 1.0f;
            _regX = (central->Pos.x - vp->Pos.x) * sx;
            _regY = (central->Pos.y - vp->Pos.y) * sy;
            _regW = central->Size.x * sx;
            _regH = central->Size.y * sy;
        }

        drawToolsPanel();
        drawPropertiesPanel();

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
            else if (_toolMode == ToolMode::Wave)
                _artwork.previewWave(dragStart, dragEnd, _waveSettings);
        }

        // Re-tessellate and re-upload only when the ink geometry changed.
        if (_artwork.isDirty())
        {
            _renderer->updateBatch(_artwork.getPolygons());
            _artwork.clearDirty();
        }
        _renderer->drawBatchSymmetric(_symmetry, view.vpX, view.vpY, view.vpW, view.vpH,
                                      static_cast<float>(_artwork.width()),
                                      static_cast<float>(_artwork.height()), _symmetryFold);

        if (_isDragging)
        {
            if (_toolMode == ToolMode::Tine || _toolMode == ToolMode::Wave)
            {
                // Draw a single drag line for the tine/wave direction
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

// Impose the default docked layout (left tool rail, right properties panel,
// canvas in the passthrough centre). Only runs when there is no saved layout.
void Application::buildDefaultDockLayout(unsigned int dockspaceId)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId,
                              ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID center = dockspaceId;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.17f, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.34f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Tools", left);
    ImGui::DockBuilderDockWindow("Properties", right);
    ImGui::DockBuilderFinish(dockspaceId);
}

// Full-width tool toggle: label on the left, shortcut hint on the right, with
// the accent fill when it is the active tool.
bool Application::toolButton(const char *label, const char *shortcut, ToolMode mode)
{
    const bool active = (_toolMode == mode);
    const ImGuiStyle &style = ImGui::GetStyle();
    const float h = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    if (active)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(Theme::Accent.x, Theme::Accent.y, Theme::Accent.z, 0.50f));

    const bool clicked = ImGui::Selectable(label, active, 0, ImVec2(0.0f, h));

    if (active)
        ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (shortcut && *shortcut)
    {
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mx = ImGui::GetItemRectMax();
        const ImVec2 ts = ImGui::CalcTextSize(shortcut);
        const ImU32 col = ImGui::GetColorU32(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(mx.x - ts.x - style.FramePadding.x, mn.y + (h - ts.y) * 0.5f), col, shortcut);
    }

    if (clicked)
        _toolMode = mode;
    return clicked;
}

void Application::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Project...", "Ctrl+S"))
            promptSaveProject();
        if (ImGui::MenuItem("Load Project..."))
            promptLoadProject();
        ImGui::Separator();
        if (ImGui::MenuItem("Export PNG..."))
            promptExportPng();
        if (ImGui::MenuItem("Export SVG..."))
            promptExportSvg();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, _artwork.canUndo()))
            _artwork.undo();
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, _artwork.canRedo()))
            _artwork.redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Clear Canvas", "Backspace"))
            _artwork.clear();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Reset View", "0"))
            resetView();
        ImGui::Separator();
        if (ImGui::MenuItem("Symmetry: None", nullptr, _symmetry == Symmetry::None))
            _symmetry = Symmetry::None;
        if (ImGui::MenuItem("Symmetry: Mirror X", nullptr, _symmetry == Symmetry::MirrorX))
            _symmetry = Symmetry::MirrorX;
        if (ImGui::MenuItem("Symmetry: Mirror Y", nullptr, _symmetry == Symmetry::MirrorY))
            _symmetry = Symmetry::MirrorY;
        if (ImGui::MenuItem("Symmetry: Quad", nullptr, _symmetry == Symmetry::Quad))
            _symmetry = Symmetry::Quad;
        if (ImGui::MenuItem("Symmetry: Radial", nullptr, _symmetry == Symmetry::Radial))
            _symmetry = Symmetry::Radial;
        ImGui::SetNextItemWidth(160.0f);
        ImGui::SliderInt("Radial folds", &_symmetryFold, 2, 16);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Patterns"))
    {
        if (ImGui::MenuItem("Stone (scatter)"))
            applyStone();
        if (ImGui::MenuItem("Nonpareil"))
            applyNonpareil();
        if (ImGui::MenuItem("Gel-git"))
            applyGelGit();
        if (ImGui::MenuItem("Chevron"))
            applyChevron();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void Application::drawToolsPanel()
{
    if (ImGui::Begin("Tools"))
    {
        if (_fonts.h1)
            ImGui::PushFont(_fonts.h1);
        ImGui::TextUnformatted("Marbling");
        if (_fonts.h1)
            ImGui::PopFont();
        ImGui::TextColored(Theme::TextMuted, "Studio");
        ImGui::Spacing();

        ImGui::SeparatorText("Tools");
        toolButton("Drop", "D", ToolMode::Drop);
        toolButton("Tine", "T", ToolMode::Tine);
        toolButton("Comb", "C", ToolMode::Comb);
        toolButton("Vortex", "V", ToolMode::Vortex);
        toolButton("Wave", "W", ToolMode::Wave);
        toolButton("Eyedropper", "E", ToolMode::Eyedropper);

        ImGui::Spacing();
        ImGui::SeparatorText("Edit");
        const float avail = ImGui::GetContentRegionAvail().x;
        const float halfW = (avail - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        ImGui::BeginDisabled(!_artwork.canUndo());
        if (ImGui::Button("Undo", ImVec2(halfW, 0.0f)))
            _artwork.undo();
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!_artwork.canRedo());
        if (ImGui::Button("Redo", ImVec2(-FLT_MIN, 0.0f)))
            _artwork.redo();
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::SeparatorText("Quick actions");
        if (ImGui::Button("Add Random Drop", ImVec2(-FLT_MIN, 0.0f)))
            addRandomDrop();
        if (ImGui::Button("Reset View", ImVec2(-FLT_MIN, 0.0f)))
            resetView();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("Clear Canvas", ImVec2(-FLT_MIN, 0.0f)))
            _artwork.clear();
        ImGui::PopStyleColor(2);
    }
    ImGui::End();
}

void Application::drawPropertiesPanel()
{
    if (!ImGui::Begin("Properties"))
    {
        ImGui::End();
        return;
    }

    // Bold, DefaultOpen collapsing header helper for consistent hierarchy.
    auto header = [&](const char *label, bool defaultOpen) -> bool
    {
        if (_fonts.bold)
            ImGui::PushFont(_fonts.bold);
        const bool open = ImGui::CollapsingHeader(
            label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        if (_fonts.bold)
            ImGui::PopFont();
        return open;
    };

    const char *toolNames[] = {"Drop", "Tine", "Comb", "Vortex", "Wave", "Eyedropper"};
    const char *toolDesc[] = {
        "Click to place ink. Hold for concentric rings.",
        "Drag to draw a single tine stroke.",
        "Drag to pull a row of evenly spaced tines.",
        "Click to swirl the ink into a vortex.",
        "Drag for a rhythmic sine-wave comb.",
        "Click the canvas to sample a color."};
    const int tool = static_cast<int>(_toolMode);

    // --- Contextual settings for the active tool ---
    if (_fonts.h1)
        ImGui::PushFont(_fonts.h1);
    ImGui::TextUnformatted(toolNames[tool]);
    if (_fonts.h1)
        ImGui::PopFont();
    ImGui::TextColored(Theme::TextMuted, "%s", toolDesc[tool]);
    ImGui::Spacing();

    switch (_toolMode)
    {
    case ToolMode::Drop:
        ImGui::SliderFloat("Radius", &_dropRadius, 10.0f, 150.0f, "%.0f px");
        ImGui::ColorEdit3("Color", (float *)&_nextDropColor);
        ImGui::Checkbox("Concentric rings", &_concentricDrop);
        if (_concentricDrop)
            ImGui::SliderInt("Rings", &_ringCount, 2, 10);
        break;
    case ToolMode::Tine:
        ImGui::SliderFloat("Strength (z)", &_tineSettings.z, 10.0f, 200.0f);
        ImGui::SliderFloat("Falloff (c)", &_tineSettings.c, 1.0f, 50.0f);
        break;
    case ToolMode::Comb:
        ImGui::SliderInt("Tines", &_combSettings.numTines, 2, 50);
        ImGui::SliderFloat("Spacing", &_combSettings.spacing, 5.0f, 100.0f, "%.0f px");
        ImGui::SliderFloat("Strength (z)", &_combSettings.properties.z, 10.0f, 200.0f);
        ImGui::SliderFloat("Falloff (c)", &_combSettings.properties.c, 1.0f, 50.0f);
        break;
    case ToolMode::Vortex:
        ImGui::SliderFloat("Strength (z)", &_vortexSettings.z, 50.0f, 500.0f);
        ImGui::SliderFloat("C-Value (c)", &_vortexSettings.c, 10.0f, 100.0f);
        ImGui::SliderFloat("Radius (r)", &_vortexSettings.r, 10.0f, 100.0f);
        break;
    case ToolMode::Wave:
        ImGui::SliderFloat("Amplitude", &_waveSettings.amplitude, 2.0f, 120.0f, "%.0f px");
        ImGui::SliderFloat("Wavelength", &_waveSettings.wavelength, 20.0f, 400.0f, "%.0f px");
        break;
    case ToolMode::Eyedropper:
        ImGui::TextDisabled("No settings. Sampled color becomes the drop color.");
        break;
    }
    ImGui::Spacing();

    // --- Palette ---
    if (header("Palette", true))
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

            // Clickable swatches that set the active drop color; they wrap to
            // the panel width.
            const std::vector<Color> &colors = _palettes[_activePalette].colors;
            const float swatch = 26.0f;
            const float visibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            const ImGuiStyle &style = ImGui::GetStyle();
            for (int i = 0; i < static_cast<int>(colors.size()); ++i)
            {
                ImGui::PushID(i);
                const ImVec4 col(colors[i].r, colors[i].g, colors[i].b, 1.0f);
                if (ImGui::ColorButton("##swatch", col, ImGuiColorEditFlags_NoTooltip,
                                       ImVec2(swatch, swatch)))
                    _nextDropColor = colors[i];
                ImGui::PopID();

                const float nextX = ImGui::GetItemRectMax().x + style.ItemSpacing.x + swatch;
                if (i + 1 < static_cast<int>(colors.size()) && nextX < visibleX)
                    ImGui::SameLine();
            }
        }

        ImGui::ColorEdit3("Drop Color", (float *)&_nextDropColor);
        ImGui::Checkbox("Random drops use palette", &_randomUsesPalette);

        ImGui::Spacing();
        // Nondestructive recolor: re-skin the whole piece to this palette.
        if (!_palettes.empty() && ImGui::Button("Recolor to palette"))
            _artwork.setRecolor(_palettes[_activePalette].colors);
        ImGui::SameLine();
        if (ImGui::Button("Restore colors"))
            _artwork.clearRecolor();
        if (_artwork.isRecolorActive())
            ImGui::TextColored(Theme::Accent, "Recolor active (originals preserved)");
    }

    // --- Patterns ---
    if (header("Patterns", false))
    {
        ImGui::SliderFloat("Comb spacing", &_patternSpacing, 10.0f, 120.0f, "%.0f px");
        ImGui::SliderInt("Stone drops", &_stoneDrops, 5, 200);
        const float avail = ImGui::GetContentRegionAvail().x;
        const float halfW = (avail - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button("Stone", ImVec2(halfW, 0.0f)))
            applyStone();
        ImGui::SameLine();
        if (ImGui::Button("Nonpareil", ImVec2(-FLT_MIN, 0.0f)))
            applyNonpareil();
        if (ImGui::Button("Gel-git", ImVec2(halfW, 0.0f)))
            applyGelGit();
        ImGui::SameLine();
        if (ImGui::Button("Chevron", ImVec2(-FLT_MIN, 0.0f)))
            applyChevron();
    }

    // --- Canvas ---
    if (header("Canvas", true))
    {
        ImGui::ColorEdit3("Background", (float *)&_backgroundColor);
        ImGui::ColorEdit3("Matte", (float *)&_outsideColor);
        helpMarker("Color of the area around the canvas frame.");

        ImGui::InputInt2("Size", _canvasSize);

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
        ImGui::TextDisabled("clears the piece");
    }

    // --- Project ---
    if (header("Project", false))
    {
        ImGui::InputText("Path", _projectPath, sizeof(_projectPath));
        const float avail = ImGui::GetContentRegionAvail().x;
        const float halfW = (avail - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button("Save", ImVec2(halfW, 0.0f)))
            promptSaveProject();
        ImGui::SameLine();
        if (ImGui::Button("Load", ImVec2(-FLT_MIN, 0.0f)))
            promptLoadProject();
        if (!_projectStatus.empty())
            ImGui::TextWrapped("%s", _projectStatus.c_str());
    }

    // --- Export ---
    if (header("Export", false))
    {
        ImGui::InputText("PNG file", _exportPath, sizeof(_exportPath));
        ImGui::SliderFloat("Scale", &_exportScale, 0.5f, 8.0f, "%.1fx");

        if (ImGui::SmallButton("1x"))
            _exportScale = 1.0f;
        ImGui::SameLine();
        if (ImGui::SmallButton("2x"))
            _exportScale = 2.0f;
        ImGui::SameLine();
        if (ImGui::SmallButton("4x"))
            _exportScale = 4.0f;
        ImGui::SameLine();
        if (ImGui::SmallButton("Print (300dpi A4)"))
        {
            const int longEdge = std::max(_artwork.width(), _artwork.height());
            _exportScale = 3508.0f / static_cast<float>(longEdge);
        }

        ImGui::Checkbox("Transparent background", &_exportTransparent);
        ImGui::TextDisabled("Output: %d x %d px",
                            static_cast<int>(_artwork.width() * _exportScale),
                            static_cast<int>(_artwork.height() * _exportScale));
        if (ImGui::Button("Export PNG", ImVec2(-FLT_MIN, 0.0f)))
            promptExportPng();

        ImGui::Spacing();
        ImGui::InputText("SVG file", _svgPath, sizeof(_svgPath));
        if (ImGui::Button("Export SVG (vector)", ImVec2(-FLT_MIN, 0.0f)))
            promptExportSvg();
        ImGui::TextDisabled("SVG ignores symmetry");

        ImGui::Spacing();
        ImGui::InputText("Timelapse dir", _timelapseDir, sizeof(_timelapseDir));
        if (FileDialog::available())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Browse##tl"))
            {
                const std::string d = FileDialog::folder("Timelapse Output Folder");
                if (!d.empty())
                    copyToBuffer(_timelapseDir, sizeof(_timelapseDir), d);
            }
        }
        ImGui::SliderInt("Frames", &_timelapseFrames, 10, 300);
        if (ImGui::Button("Export Timelapse", ImVec2(-FLT_MIN, 0.0f)))
            exportTimelapse();

        if (!_exportStatus.empty())
            ImGui::TextWrapped("%s", _exportStatus.c_str());
    }

    ImGui::End();
}

void Application::drawStatusBar()
{
    ImGuiViewport *vp = ImGui::GetMainViewport();
    const float h = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

    const bool open = ImGui::BeginViewportSideBar("##StatusBar", vp, ImGuiDir_Down, h, flags);
    if (open && ImGui::BeginMenuBar())
    {
        const char *toolNames[] = {"Drop", "Tine", "Comb", "Vortex", "Wave", "Eyedropper"};
        auto sep = []() { ImGui::SameLine(0.0f, 14.0f); ImGui::TextDisabled("|"); ImGui::SameLine(0.0f, 14.0f); };

        ImGui::Text("Tool: %s", toolNames[static_cast<int>(_toolMode)]);
        sep();
        ImGui::Text("%d x %d", _artwork.width(), _artwork.height());
        sep();
        ImGui::Text("Zoom %.0f%%", _zoom * 100.0f);
        sep();
        ImGui::Text("Ops %d", static_cast<int>(_artwork.operations().size()));
        if (_artwork.isRecolorActive())
        {
            sep();
            ImGui::TextColored(Theme::Accent, "Recolor");
        }

        // Right-aligned FPS.
        char fps[32];
        std::snprintf(fps, sizeof(fps), "%.0f FPS  %.2f ms", ImGui::GetIO().Framerate,
                      1000.0f / ImGui::GetIO().Framerate);
        const float tw = ImGui::CalcTextSize(fps).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - tw - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::TextDisabled("%s", fps);

        ImGui::EndMenuBar();
    }
    ImGui::End();
}