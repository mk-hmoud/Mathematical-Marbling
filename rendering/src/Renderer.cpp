#include "Renderer.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "earcut.hpp"
#include "TessAdapter.h"

const char *VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;

    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);
    }
)";

const char *FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec4 drawColor;

    void main()
    {
        FragColor = drawColor;
    }
)";

Renderer::Renderer()
    : _shader(VERTEX_SHADER_SOURCE, FRAGMENT_SHADER_SOURCE)
{
    // core OpenGL objects
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    // Set up the Vertex Array Object (VAO)
    // The VAO stores this configuration, so we only do it once.
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    // Tell the VAO how to read the VBO:
    //    - Attribute 0 (aPos in the shader)
    //    - Has 2 components (x, y)
    //    - Of type GL_FLOAT
    //    - Not normalized
    //    - Stride is sizeof(Point)
    //    - Offset is 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind objects
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    // The _shader object's destructor will also run,
    // but its ID will be cleaned up by OpenGL context destruction
    // Possibly make this intentional by setting up a Shader destructor to do this.
}

void Renderer::setupProjection(int width, int height)
{
    glViewport(0, 0, width, height);
    _projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
}

void Renderer::drawPolygons(const std::vector<Polygon> &polygons)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Activate our shader and send it the project matrix
    _shader.use();
    _shader.setMat4("projection", _projection);

    // Bind our VAO (which remembers our VBO layout) and our EBO
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

    for (const auto &poly : polygons)
    {
        // Prepare polygon data for earcut
        std::vector<std::vector<Point>> earcutInput;
        earcutInput.push_back(poly.vertices);

        // Run CPU tessellation. This generates a list of triangle indices
        //    that refer to the polygon's original vertex list.
        std::vector<uint32_t> indices = mapbox::earcut(earcutInput);

        // Upload the polygon's vertex data to the VBO (GPU memory)
        glBufferData(GL_ARRAY_BUFFER, poly.vertices.size() * sizeof(Point), poly.vertices.data(), GL_DYNAMIC_DRAW);

        // Upload the triangle index data to the EBO (GPU memory)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);

        // Set the color for this polygon in the fragment shader
        _shader.setVec4("drawColor", poly.color.r, poly.color.g, poly.color.b, 1.0f);

        // Issue the final draw call!
        //    - Draw Triangles
        //    - Use `indices.size()` number of indices
        //    - The indices are 32-bit unsigned ints
        //    - Start from the beginning of the EBO (nullptr)
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, (void *)0);
    }

    // Unbind everything
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void Renderer::drawDragLine(const Point &start, const Point &end)
{
    // Activate shader and set uniforms
    _shader.use();
    _shader.setMat4("projection", _projection);
    _shader.setVec4("drawColor", 1.0f, 1.0f, 1.0f, 1.0f); // White

    // Create a temporary buffer with just the two line points
    Point vertices[2] = {start, end};

    // Bind our VAO and VBO
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    // Upload the line data (overwriting whatever was in the VBO)
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // Unbind the EBO, since we aren't using indices for this draw
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glLineWidth(2.0f);

    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
}