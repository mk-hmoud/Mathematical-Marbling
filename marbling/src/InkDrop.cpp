#include "InkDrop.h"

#include <cmath>

constexpr int NUM_VERTICES = 512 - (512 - 256) / 2;

InkDrop::InkDrop(float x, float y, float radius, float r, float g, float b)
    : _centre(x, y), _radius(radius), _r(r), _g(g), _b(b)
{
    _vertices.reserve(NUM_VERTICES);
    float angleStep = 2.0f * M_PI / NUM_VERTICES;

    for (int i = 0; i < NUM_VERTICES; i++)
    {
        float currentAngle = i * angleStep;

        float pointX = _centre.x + radius * cosf(currentAngle);
        float pointY = _centre.y + radius * sinf(currentAngle);

        _vertices.emplace_back(pointX, pointY);
    }
}

void InkDrop::marbled(const InkDrop &other)
{
    Point c(other.getCentre());
    for (auto &p : _vertices)
    {
        Point displacement = p - c;
        float magnitude = std::sqrt(displacement.x * displacement.x + displacement.y * displacement.y);

        float scaling = std::sqrt(1.0f + (other._radius * other._radius) / (magnitude * magnitude));
        p = c + displacement * scaling;
    }
}

void InkDrop::tine(const Point &m, float x, float y, float z, float c)
{
    float u = 1.0f / std::pow(2.0f, 1.0f / c);
    Point b(x, y);

    for (auto &v : _vertices)
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
    for (auto &v : _vertices)
    {
        float u = 1.0f / std::pow(2.0f, 1.0f / c);
        // =distance from the vertex 'v' to the vortex center 'pos'
        float h = std::sqrt(std::pow(v.x - pos.x, 2) + std::pow(v.y - pos.y, 2));

        if (h == 0.0f)
            continue;

        float i = z * std::pow(u, -r) * std::pow(u, h);
        float a = i / h; // Rotation angle

        Point translatedV = Point(v.x - pos.x, v.y - pos.y);

        // 2D rotation
        float rotatedX = translatedV.x * std::cos(a) - translatedV.y * std::sin(a);
        float rotatedY = translatedV.x * std::sin(a) + translatedV.y * std::cos(a);

        v.x = pos.x + rotatedX;
        v.y = pos.y + rotatedY;
    }
}

void InkDrop::getColor(float &red, float &green, float &blue) const
{
    red = _r;
    green = _g;
    blue = _b;
}

const std::vector<Point> &InkDrop::getVertices() const
{
    return _vertices;
}

Point InkDrop::getCentre() const
{
    return _centre;
}

float InkDrop::getRadius() const
{
    return _radius;
}
