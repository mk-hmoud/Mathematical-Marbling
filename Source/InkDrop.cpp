#include "InkDrop.h"
#include <cmath>
#include <algorithm>

constexpr int NUM_VERTICES = 1024;

InkDrop::InkDrop(float x, float y, float radius, float r, float g, float b)
    : centre(x, y), radius(radius), r(r), g(g), b(b)
{

    float epsilon = 0.05f;
    float currentX = radius;
    float currentY = 0.0f;

    // Minsky circle algorithm
    for (int i = 0; i < NUM_VERTICES; i++)
    {
        vertices.emplace_back(centre.x + currentX, centre.y + currentY);

        float newX = currentX - epsilon * currentY;
        float newY = currentY + epsilon * newX;

        currentX = newX;
        currentY = newY;
    }
}

void InkDrop::render() const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glColor4f(r, g, b, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);

    std::vector<float> vertexData;
    for (const auto &vertex : vertices)
    {
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
    }

    glVertexPointer(2, GL_FLOAT, 0, vertexData.data());
    // Render points only
    glPointSize(2.0f);
    glDrawArrays(GL_POINTS, 0, NUM_VERTICES);

    // Render Polygon
    // glDrawArrays(GL_POLYGON, 0, NUM_VERTICES);

    glDisableClientState(GL_VERTEX_ARRAY);

    glLineWidth(1.5f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glDrawArrays(GL_LINE_LOOP, 0, NUM_VERTICES);

    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

void InkDrop::marbled(const InkDrop &other)
{
    Point c(other.getX(), other.getY());
    for (auto &p : vertices)
    {
        Point displacement = p - c;
        float magnitude = std::sqrt(displacement.x * displacement.x + displacement.y * displacement.y);

        float scaling = std::sqrt(1.0f + (other.radius * other.radius) / (magnitude * magnitude));
        p = c + displacement * scaling;
    }
}

void InkDrop::tine(const Point &m, float x, float y, float z, float c)
{
    float u = 1.0f / std::pow(2.0f, 1.0f / c);
    Point b(x, y);

    for (auto &v : vertices)
    {
        Point pb(v.x - b.x, v.y - b.y);
        Point n(-m.y, m.x);

        float d = std::abs(pb.x * n.x + pb.y * n.y);
        float mag = z * std::pow(u, d);

        v.x += m.x * mag;
        v.y += m.y * mag;
    }
}

void InkDrop::vortex(const Point &pos, float z, float c, float r)
{
    for (auto &v : vertices)
    {
        Point P = v;
        Point C = pos;

        float u = 1.0f / std::pow(2.0f, 1.0f / c);
        float h = std::sqrt(std::pow(P.x - C.x, 2) + std::pow(P.y - C.y, 2));
        float i = z * std::pow(u, -r) * std::pow(u, h);
        float a = i / std::abs(h);

        Point translatedPb = Point(P.x - C.x, P.y - C.y);

        float rotatedX = translatedPb.x * std::cos(a) + translatedPb.y * std::sin(a);
        float rotatedY = -translatedPb.x * std::sin(a) + translatedPb.y * std::cos(a);

        Point rotatedTranslatedP = Point(rotatedX, rotatedY);
        C.x += rotatedTranslatedP.x;
        C.y += rotatedTranslatedP.y;

        v = Point(C.x, C.y);
    }
}

float InkDrop::getX() const
{
    return centre.x;
}

float InkDrop::getY() const
{
    return centre.y;
}

float InkDrop::getRadius() const
{
    return radius;
}

InkDrop::~InkDrop() {}

void InkDrop::fill() const
{
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;

    for (const auto &vertex : vertices)
    {
        if (vertex.x < minX)
            minX = vertex.x;
        if (vertex.x > maxX)
            maxX = vertex.x;
        if (vertex.y < minY)
            minY = vertex.y;
        if (vertex.y > maxY)
            maxY = vertex.y;
    }

    for (float y = minY; y <= maxY; y++)
    {
        std::vector<float> intersections;

        for (size_t i = 0; i < vertices.size(); i++)
        {
            size_t next = (i + 1) % vertices.size();
            const Point &p1 = vertices[i];
            const Point &p2 = vertices[next];

            if ((y >= p1.y && y < p2.y) || (y >= p2.y && y < p1.y))
            {
                float xIntersection = p1.x + (y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                intersections.push_back(xIntersection);
            }
        }
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i < intersections.size(); i += 2)
        {
            float xStart = intersections[i];
            float xEnd = intersections[i + 1];

            for (float x = xStart; x <= xEnd; x++)
            {
                drawPixel(x, y);
            }
        }
    }
}

void InkDrop::drawPixel(float x, float y) const
{
    glColor3f(1, 0, 0);
    glBegin(GL_POINTS);
    glVertex2f(x, y);
    glEnd();
}
