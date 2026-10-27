#include "Renderer.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

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

void Renderer::setView(int x, int y, int width, int height, float coordWidth, float coordHeight)
{
    glViewport(x, y, width, height);
    _projection = glm::ortho(0.0f, coordWidth, coordHeight, 0.0f, -1.0f, 1.0f);
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

void Renderer::drawBatchWithModel(const glm::mat4 &model)
{
    if (_indexCount == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _polygonShader.use();
    _polygonShader.setMat4("projection", _projection * model);

    glBindVertexArray(_polyVao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Renderer::drawBatchSymmetric(Symmetry sym, int vpX, int vpY, int vpW, int vpH,
                                  float coordW, float coordH)
{
    if (sym == Symmetry::None)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(vpX, vpY, vpW, vpH);
        drawBatch();
        return;
    }

    // Mirror transforms in canvas coordinates.
    const glm::mat4 I(1.0f);
    const glm::mat4 mx = glm::translate(glm::mat4(1.0f), glm::vec3(coordW, 0.0f, 0.0f)) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    const glm::mat4 my = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, coordH, 0.0f)) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
    const glm::mat4 mxy = glm::translate(glm::mat4(1.0f), glm::vec3(coordW, coordH, 0.0f)) *
                          glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 1.0f));

    const int halfW = vpW / 2;
    const int halfH = vpH / 2;
    const int midX = vpX + halfW;
    const int midY = vpY + halfH; // GL framebuffer center (bottom-left origin)

    glEnable(GL_SCISSOR_TEST);

    auto region = [&](int x, int y, int w, int h, const glm::mat4 &m)
    {
        glScissor(x, y, w, h);
        drawBatchWithModel(m);
    };

    if (sym == Symmetry::MirrorX)
    {
        region(vpX, vpY, halfW, vpH, I);                 // left = source
        region(midX, vpY, vpW - halfW, vpH, mx);         // right = mirror of left
    }
    else if (sym == Symmetry::MirrorY)
    {
        region(vpX, midY, vpW, vpH - halfH, I);          // top = source
        region(vpX, vpY, vpW, halfH, my);                // bottom = mirror of top
    }
    else // Quad: top-left quadrant is the source
    {
        region(vpX, midY, halfW, vpH - halfH, I);        // top-left
        region(midX, midY, vpW - halfW, vpH - halfH, mx); // top-right
        region(vpX, vpY, halfW, halfH, my);              // bottom-left
        region(midX, vpY, vpW - halfW, halfH, mxy);      // bottom-right
    }

    // Leave the scissor box set to the full canvas rect for any overlay.
    glScissor(vpX, vpY, vpW, vpH);
}

std::vector<unsigned char> Renderer::renderToPixels(const std::vector<Polygon> &polygons,
                                                    int pixelWidth, int pixelHeight,
                                                    float coordWidth, float coordHeight,
                                                    const Color &background,
                                                    Symmetry symmetry,
                                                    bool transparent)
{
    if (pixelWidth <= 0 || pixelHeight <= 0)
        return {};

    // Clamp to what the driver can allocate, so an over-large export fails
    // gracefully (scaled down) instead of producing an incomplete framebuffer.
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
    if (maxSize > 0)
    {
        if (pixelWidth > maxSize)
            pixelWidth = maxSize;
        if (pixelHeight > maxSize)
            pixelHeight = maxSize;
    }

    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    const GLsizei samples = static_cast<GLsizei>(maxSamples >= 4 ? 4 : maxSamples);

    // --- Multisampled target to render into (smooth edges) ---
    GLuint msFbo = 0, msColor = 0;
    glGenFramebuffers(1, &msFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, msFbo);
    glGenRenderbuffers(1, &msColor);
    glBindRenderbuffer(GL_RENDERBUFFER, msColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, pixelWidth, pixelHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msColor);

    std::vector<unsigned char> pixels;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        glViewport(0, 0, pixelWidth, pixelHeight);
        _projection = glm::ortho(0.0f, coordWidth, coordHeight, 0.0f, -1.0f, 1.0f);
        if (transparent)
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        else
            glClearColor(background.r, background.g, background.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        updateBatch(polygons);
        drawBatchSymmetric(symmetry, 0, 0, pixelWidth, pixelHeight, coordWidth, coordHeight);
        glDisable(GL_SCISSOR_TEST);

        // --- Resolve the multisampled image into a regular texture FBO ---
        GLuint resolveFbo = 0, resolveTex = 0;
        glGenTextures(1, &resolveTex);
        glBindTexture(GL_TEXTURE_2D, resolveTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pixelWidth, pixelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &resolveFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTex, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
        glBlitFramebuffer(0, 0, pixelWidth, pixelHeight, 0, 0, pixelWidth, pixelHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // --- Read the resolved pixels back ---
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        const size_t rowBytes = static_cast<size_t>(pixelWidth) * 4;
        std::vector<unsigned char> gl(static_cast<size_t>(pixelHeight) * rowBytes);
        glReadPixels(0, 0, pixelWidth, pixelHeight, GL_RGBA, GL_UNSIGNED_BYTE, gl.data());

        // OpenGL returns bottom-to-top; flip to top-first for image files.
        pixels.resize(gl.size());
        for (int y = 0; y < pixelHeight; ++y)
        {
            const unsigned char *src = gl.data() + static_cast<size_t>(pixelHeight - 1 - y) * rowBytes;
            std::copy(src, src + rowBytes, pixels.data() + static_cast<size_t>(y) * rowBytes);
        }

        glDeleteFramebuffers(1, &resolveFbo);
        glDeleteTextures(1, &resolveTex);
    }

    // Restore the default framebuffer; the main loop resets viewport/projection.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &msColor);
    glDeleteFramebuffers(1, &msFbo);

    return pixels;
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