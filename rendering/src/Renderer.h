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
     * @brief Renders a list of complex, colored polygons.
     * * This function uses `earcut.hpp` to tessellate the polygons
     * on the CPU, then uploads the resulting vertices/indices
     * to the GPU for efficient rendering.
     *
     * @param polygons A list of `Polygon` objects to be drawn.
     */
    void drawPolygons(const std::vector<Polygon> &polygons);

    /**
     * @brief Renders a simple, single-colored line.
     * @param start The starting point of the line.
     * @param end The ending point of the line.
     */
    void drawDragLine(const Point &start, const Point &end);

    /**
     * @brief Calculates and sets the 2D orthographic projection matrix.
     * This replaces the legacy `glOrtho` call.
     * @param width The width of the viewport (in pixels).
     * @param height The height of the viewport (in pixels).
     */
    void setupProjection(int width, int height);

private:
    Shader _shader;
    glm::mat4 _projection;

    GLuint _vao;
    GLuint _vbo;
    GLuint _ebo;
};

#endif // RENDERER_H