#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "DataTypes.h"
#include "Shader.h"
#include <glm/glm.hpp>

/**
 * @class Renderer
 * @brief Main rendering engine for the application.
 *
 * This class handles all modern OpenGL drawing. It is decoupled
 * from the `marbling` simulation and only knows how to draw
 * generic `Polygon` objects. It manages all required OpenGL
 * objects (VAO, VBOs, Shaders).
 */

class Renderer
{
public:
    Renderer();
    ~Renderer();

    /**
     * @brief Re-tessellates the polygons and uploads them to the GPU as a
     * single batched vertex/index buffer (position + per-vertex color).
     *
     * This is the expensive step (CPU tessellation + GPU upload) and should
     * only be called when the geometry actually changes, not every frame.
     *
     * @param polygons A list of `Polygon` objects to cache for drawing.
     */
    void updateBatch(const std::vector<Polygon> &polygons);

    /**
     * @brief Draws the batch built by the last updateBatch() in one draw call.
     * Cheap enough to call every frame.
     */
    void drawBatch();

    /**
     * @brief Renders the polygons to an offscreen (multisampled) framebuffer at
     * an arbitrary resolution and returns the pixels, for resolution-independent
     * export. Does not touch the default framebuffer's contents.
     *
     * @param polygons    The scene to render.
     * @param pixelWidth  Output image width in pixels.
     * @param pixelHeight Output image height in pixels.
     * @param coordWidth  Width of the artwork coordinate space to map across the image.
     * @param coordHeight Height of the artwork coordinate space to map across the image.
     * @param background  Background color to clear with.
     * @return RGBA pixels, top row first (width*height*4 bytes), or empty on failure.
     */
    std::vector<unsigned char> renderToPixels(const std::vector<Polygon> &polygons,
                                              int pixelWidth, int pixelHeight,
                                              float coordWidth, float coordHeight,
                                              const Color &background);

    /**
     * @brief Renders a simple, single-colored line.
     * @param start The starting point of the line.
     * @param end The ending point of the line.
     */
    void drawDragLine(const Point &start, const Point &end);

    /**
     * @brief Renders multiple parallel lines, representing a comb.
     * @param combPoints A vector of pairs of points, where each pair (start, end) defines one tine.
     * @param color The color to draw the comb lines.
     */
    void drawCombLines(const std::vector<std::pair<Point, Point>> &combPoints, const Color &color);

    /**
     * @brief Calculates and sets the 2D orthographic projection matrix.
     * This replaces the legacy `glOrtho` call.
     * @param width The width of the viewport (in pixels).
     * @param height The height of the viewport (in pixels).
     */
    void setupProjection(int width, int height);

private:
    Shader _shader;         // uniform-color pipeline, used for lines
    Shader _polygonShader;  // per-vertex-color pipeline, used for the ink batch
    glm::mat4 _projection;

    // Line pipeline (drag line / comb preview): interleaved position only.
    GLuint _vao;
    GLuint _vbo;
    GLuint _ebo;

    // Batched ink pipeline: interleaved position + color, drawn in one call.
    GLuint _polyVao;
    GLuint _polyVbo;
    GLuint _polyEbo;
    GLsizei _indexCount = 0;
};

#endif // RENDERER_H