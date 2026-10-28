#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Artwork.h"
#include "Renderer.h"
#include "Palette.h"
#include "Theme.h"

class Renderer;

class Application
{
public:
    Application(int width, int height, const char *title);
    ~Application();
    void run();

private:
    enum class ToolMode
    {
        Drop,
        Tine,
        Comb,
        Vortex,
        Wave,
        Eyedropper
    };

    void handleMouseInput(int button, int action, int mods);
    void handleKeyInput(int key, int scancode, int action, int mods);
    void handleScroll(double xoffset, double yoffset);
    void resetView();
    void placeDrop(const Point &center); // single drop or concentric rings
    void pickColor(double windowX, double windowY); // eyedropper
    void addRandomDrop();

    // How the fixed-size canvas is placed inside the window (framebuffer px).
    struct CanvasView
    {
        float scale;             // canvas units -> framebuffer px
        float offX, offY;        // top-left of the canvas rect, from the top-left
        int vpX, vpY, vpW, vpH;  // GL viewport rect (bottom-left origin)
    };
    CanvasView canvasView(int fbWidth, int fbHeight) const;
    Point windowToCanvas(double windowX, double windowY) const;

    // UI (ImGui) — a docked "studio" layout built once per fresh session.
    void buildDefaultDockLayout(unsigned int dockspaceId);
    void drawMenuBar();
    void drawToolsPanel();      // left rail: tool selection + quick actions
    void drawPropertiesPanel(); // right: palette, settings, canvas, export
    void drawStatusBar();       // bottom: tool, zoom, size, FPS
    bool toolButton(const char *label, const char *shortcut, ToolMode mode);

    void exportPng();
    void exportSvg();
    void exportTimelapse();
    void saveProject();
    void loadProject();

    // Prompt via a native dialog (falling back to the typed path), then act.
    void promptSaveProject();
    void promptLoadProject();
    void promptExportPng();
    void promptExportSvg();

    // Authentic marbling patterns (full-canvas gestures, one undo step each).
    void applyStone();
    void applyNonpareil();
    void applyGelGit();
    void applyChevron();

private:
    GLFWwindow *_window;
    Artwork _artwork;
    std::unique_ptr<Renderer> _renderer;

    // UI theme / typography
    Theme::Fonts _fonts;
    float _dpiScale = 1.0f;

    // Region the canvas renders into (framebuffer px, top-left origin). This is
    // the dockspace central node, so panels never overlap the artwork. Zero
    // width means "not set yet" -> fall back to the whole framebuffer.
    float _regX = 0.0f, _regY = 0.0f, _regW = 0.0f, _regH = 0.0f;

    // Input state
    bool _isDragging;
    double _startX, _startY;

    // View (zoom/pan) state
    float _zoom = 1.0f;         // multiplies the fit-to-window scale
    float _panX = 0.0f;         // canvas offset in framebuffer px
    float _panY = 0.0f;
    bool _isPanning = false;
    double _panLastX = 0.0, _panLastY = 0.0;
    GLint _maxViewportDim = 8192; // queried at startup; caps zoom

    ToolMode _toolMode = ToolMode::Drop;

    VortexSettings _vortexSettings;
    TineSettings _tineSettings;
    CombSettings _combSettings;
    WaveSettings _waveSettings;

    // Pattern controls
    float _patternSpacing = 30.0f; // comb tooth spacing used by patterns
    int _stoneDrops = 40;          // drops dispensed by the Stone fill

    // GUI state
    float _dropRadius = 75.0f;
    Color _nextDropColor = {1.0f, 0.2f, 0.2f};
    Color _backgroundColor = {0.07f, 0.07f, 0.09f};
    // Color of the window area outside the canvas frame.
    Color _outsideColor = {0.16f, 0.16f, 0.18f};

    // Pending size for the "New Canvas" action (the live canvas size lives on
    // the Artwork).
    int _canvasSize[2] = {1200, 900};

    // Palette state
    std::vector<Palette> _palettes = builtinPalettes();
    int _activePalette = 0;
    bool _randomUsesPalette = true;

    // Concentric ring drops (marbled "stone"/flower primitive)
    bool _concentricDrop = false;
    int _ringCount = 4;

    // View options
    Symmetry _symmetry = Symmetry::None;
    int _symmetryFold = 6; // k for radial symmetry

    // Export state
    char _exportPath[256] = "marbling.png";
    char _svgPath[256] = "marbling.svg";
    float _exportScale = 2.0f;        // output = canvas size * scale
    bool _exportTransparent = false;  // transparent background (PNG/SVG)
    std::string _exportStatus;        // last result, shown in the UI

    // Timelapse (PNG frame sequence of the replay)
    char _timelapseDir[256] = "timelapse";
    int _timelapseFrames = 60;

    // Project (save/load) state
    char _projectPath[256] = "artwork.marble";
    std::string _projectStatus;
};

#endif // APPLICATION_H