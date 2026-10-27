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

class Renderer;

class Application
{
public:
    Application(int width, int height, const char *title);
    ~Application();
    void run();

private:
    void handleMouseInput(int button, int action, int mods);
    void handleKeyInput(int key, int scancode, int action, int mods);
    void handleScroll(double xoffset, double yoffset);
    void resetView();
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

    void exportPng();
    void saveProject();
    void loadProject();

private:
    GLFWwindow *_window;
    Artwork _artwork;
    std::unique_ptr<Renderer> _renderer;

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

    enum class ToolMode
    {
        Drop,
        Tine,
        Comb,
        Vortex
    };
    ToolMode _toolMode = ToolMode::Drop;

    VortexSettings _vortexSettings;
    TineSettings _tineSettings;
    CombSettings _combSettings;

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

    // Export state
    char _exportPath[256] = "marbling.png";
    float _exportScale = 2.0f;        // output = canvas size * scale
    std::string _exportStatus;        // last result, shown in the UI

    // Project (save/load) state
    char _projectPath[256] = "artwork.marble";
    std::string _projectStatus;
};

#endif // APPLICATION_H