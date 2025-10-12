#include "Renderer.h"

#include <GL/glew.h>
#include <GL/glu.h>
#include <vector>
#include <cstdio>

// --- Tessellation Callbacks ---
namespace
{
    void beginCallback(GLenum which) { glBegin(which); }
    void endCallback() { glEnd(); }
    void vertexCallback(GLvoid *vertex)
    {
        const GLdouble *ptr = (const GLdouble *)vertex;
        glVertex3dv(ptr);
    }
    void errorCallback(GLenum errorCode)
    {
        const GLubyte *estring = gluErrorString(errorCode);
        fprintf(stderr, "Tessellation Error: %s\n", estring);
    }
    void combineCallback(GLdouble coords[3], const GLdouble *vertex_data[4],
                         const GLfloat weight[4], GLvoid **outData)
    {
        GLdouble *newVertex = new GLdouble[3];
        newVertex[0] = coords[0];
        newVertex[1] = coords[1];
        newVertex[2] = coords[2];
        *outData = newVertex;
    }
}

Renderer::Renderer()
{
    // ..gl setup?
}

Renderer::~Renderer() {}

void Renderer::setupProjection(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::drawCanvas(const Canvas &canvas)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLUtesselator *tess = gluNewTess();
    if (!tess)
        return;

    // callbacks setup
    gluTessCallback(tess, GLU_TESS_BEGIN, (_GLUfuncptr)beginCallback);
    gluTessCallback(tess, GLU_TESS_END, (_GLUfuncptr)endCallback);
    gluTessCallback(tess, GLU_TESS_VERTEX, (_GLUfuncptr)vertexCallback);
    gluTessCallback(tess, GLU_TESS_ERROR, (_GLUfuncptr)errorCallback);
    gluTessCallback(tess, GLU_TESS_COMBINE, (_GLUfuncptr)combineCallback);

    for (const auto &drop : canvas.getDrops())
    {
        float r, g, b;
        drop.getColor(r, g, b);
        glColor4f(r, g, b, 1.0f);

        const auto &vertices = drop.getVertices();
        std::vector<GLdouble[3]> tessVertices(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            tessVertices[i][0] = vertices[i].x;
            tessVertices[i][1] = vertices[i].y;
            tessVertices[i][2] = 0.0;
        }

        gluTessBeginPolygon(tess, nullptr);
        gluTessBeginContour(tess);
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            gluTessVertex(tess, tessVertices[i], tessVertices[i]);
        }
        gluTessEndContour(tess);
        gluTessEndPolygon(tess);
    }

    gluDeleteTess(tess);
    glDisable(GL_BLEND);
}

void Renderer::drawDragLine(const Point &start, const Point &end)
{
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2d(start.x, start.y);
    glVertex2d(end.x, end.y);
    glEnd();
}