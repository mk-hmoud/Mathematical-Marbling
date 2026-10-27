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

// Batched ink pipeline: position + per-vertex color, so the whole scene can be
// drawn in a single call without a per-polygon color uniform.
const char *POLY_VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;

    uniform mat4 projection;

    out vec3 vColor;

    void main()
    {
        gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);
        vColor = aColor;
    }
)";

const char *POLY_FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    in vec3 vColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(vColor, 1.0);
    }
)";

Renderer::Renderer()
    : _shader(VERTEX_SHADER_SOURCE, FRAGMENT_SHADER_SOURCE),
      _polygonShader(POLY_VERTEX_SHADER_SOURCE, POLY_FRAGMENT_SHADER_SOURCE)
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

    // --- Batched ink pipeline (position + color interleaved) ---
    glGenVertexArrays(1, &_polyVao);
    glGenBuffers(1, &_polyVbo);
    glGenBuffers(1, &_polyEbo);

    glBindVertexArray(_polyVao);
    glBindBuffer(GL_ARRAY_BUFFER, _polyVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _polyEbo); // recorded in the VAO state

    const GLsizei stride = 5 * sizeof(float); // x, y, r, g, b
    // Attribute 0: position (vec2) at offset 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    // Attribute 1: color (vec3) at offset 2 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    glDeleteVertexArrays(1, &_polyVao);
    glDeleteBuffers(1, &_polyVbo);
    glDeleteBuffers(1, &_polyEbo);
    // The _shader object's destructor will also run,
    // but its ID will be cleaned up by OpenGL context destruction
    // Possibly make this intentional by setting up a Shader destructor to do this.
}

void Renderer::setupProjection(int width, int height)
{
    glViewport(0, 0, width, height);
    _projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
}

void Renderer::updateBatch(const std::vector<Polygon> &polygons)
{
    // Build one interleaved vertex buffer [x, y, r, g, b] and one index buffer
    // for the whole scene, tessellating each polygon once.
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    size_t totalVerts = 0;
    for (const auto &poly : polygons)
        totalVerts += poly.vertices.size();
    vertexData.reserve(totalVerts * 5);
    indexData.reserve(totalVerts * 3);

    uint32_t baseVertex = 0;
    std::vector<std::vector<Point>> earcutInput(1);
    for (const auto &poly : polygons)
    {
        // Tessellate this polygon (simple closed loop, no holes).
        earcutInput[0] = poly.vertices;
        std::vector<uint32_t> localIndices = mapbox::earcut(earcutInput);

        for (const auto &v : poly.vertices)
        {
            vertexData.push_back(v.x);
            vertexData.push_back(v.y);
            vertexData.push_back(poly.color.r);
            vertexData.push_back(poly.color.g);
            vertexData.push_back(poly.color.b);
        }

        // Offset the per-polygon indices into the combined vertex array.
        for (uint32_t idx : localIndices)
            indexData.push_back(baseVertex + idx);

        baseVertex += static_cast<uint32_t>(poly.vertices.size());
    }

    _indexCount = static_cast<GLsizei>(indexData.size());

    glBindVertexArray(_polyVao);
    glBindBuffer(GL_ARRAY_BUFFER, _polyVbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _polyEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(uint32_t), indexData.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

void Renderer::drawBatch()
{
    if (_indexCount == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _polygonShader.use();
    _polygonShader.setMat4("projection", _projection);

    glBindVertexArray(_polyVao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, (void *)0);
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

void Renderer::drawCombLines(const std::vector<std::pair<Point, Point>> &combPoints, const Color &color)
{
    if (combPoints.empty())
        return;

    _shader.use();
    _shader.setMat4("projection", _projection);
    _shader.setVec4("drawColor", color.r, color.g, color.b, 1.0f);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Not using EBO for lines

    // We need to flatten the vector of pairs into a single vector of Points
    std::vector<Point> allLineVertices;
    allLineVertices.reserve(combPoints.size() * 2);
    for (const auto &line : combPoints)
    {
        allLineVertices.push_back(line.first);
        allLineVertices.push_back(line.second);
    }

    glBufferData(GL_ARRAY_BUFFER, allLineVertices.size() * sizeof(Point), allLineVertices.data(), GL_DYNAMIC_DRAW);

    glLineWidth(1.0f); // Default line width for the comb

    glDrawArrays(GL_LINES, 0, (GLsizei)allLineVertices.size());

    glBindVertexArray(0);
}