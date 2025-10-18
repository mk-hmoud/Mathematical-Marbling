#include "Renderer.h"

#include <GL/glew.h>
#include <GL/glu.h>
#include <vector>
#include <cstdio>

namespace
{
    void beginCallback(GLenum which) { glBegin(which); }
    void endCallback() { glEnd(); }
    void vertexCallback(GLvoid *vertex)
    {
        const GLdouble *ptr = static_cast<const GLdouble *>(vertex);
        glVertex3dv(ptr);
    }
    void errorCallback(GLenum errorCode)
    {
        const GLubyte *estring = gluErrorString(errorCode);
        if (estring)
        {
            fprintf(stderr, "Tessellation Error: %s\n", reinterpret_cast<const char *>(estring));
        }
    }

    std::vector<GLdouble *> g_allocs;
    void combineCallback(GLdouble coords[3], const GLdouble *vertex_data[4],
                         const GLfloat weight[4], GLvoid **outData)
    {
        GLdouble *newVertex = new GLdouble[3];
        newVertex[0] = coords[0];
        newVertex[1] = coords[1];
        newVertex[2] = coords[2];
        g_allocs.push_back(newVertex);
        *outData = newVertex;
    }
}

Renderer::Renderer() = default;

void Renderer::setupProjection(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::drawPolygons(const std::vector<Polygon> &polygons)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto &poly : polygons)
    {
        glColor4f(poly.color.r, poly.color.g, poly.color.b, 1.0f);

        g_allocs.clear();
        GLUtesselator *tess = gluNewTess();
        if (!tess)
            continue;

        gluTessCallback(tess, GLU_TESS_BEGIN, reinterpret_cast<_GLUfuncptr>(beginCallback));
        gluTessCallback(tess, GLU_TESS_END, reinterpret_cast<_GLUfuncptr>(endCallback));
        gluTessCallback(tess, GLU_TESS_VERTEX, reinterpret_cast<_GLUfuncptr>(vertexCallback));
        gluTessCallback(tess, GLU_TESS_ERROR, reinterpret_cast<_GLUfuncptr>(errorCallback));
        gluTessCallback(tess, GLU_TESS_COMBINE, reinterpret_cast<_GLUfuncptr>(combineCallback));

        const auto &vertices = poly.vertices;
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

        gluDeleteTess(tess);

        for (auto *p : g_allocs)
        {
            delete[] p;
        }
        g_allocs.clear();
    }

    glDisable(GL_BLEND);
}

void Renderer::drawDragLine(const Point &start, const Point &end)
{
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(start.x, start.y);
    glVertex2f(end.x, end.y);
    glEnd();
}