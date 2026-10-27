#include "InkDrop.h"

#include <cmath>
#include <array>
#include <algorithm>

namespace
{
    constexpr int MIN_VERTICES = 64;
    constexpr int MAX_VERTICES = 384;

    // Scale the boundary resolution with the drop's size: roughly one vertex
    // per 2px of circumference, clamped so small drops stay cheap and large
    // ones stay smooth.
    int vertexCountForRadius(float radius)
    {
        int n = static_cast<int>(static_cast<float>(M_PI) * radius); // 2*pi*r / 2
        return std::clamp(n, MIN_VERTICES, MAX_VERTICES);
    }
}

InkDrop::InkDrop(float x, float y, float radius, float r, float g, float b)
    : _initialCentre(x, y), _initialRadius(radius), _color{r, g, b}
{
    const int numVertices = vertexCountForRadius(radius);
    _boundary.reserve(numVertices);
    float angleStep = 2.0f * static_cast<float>(M_PI) / static_cast<float>(numVertices);

    for (int i = 0; i < numVertices; ++i)
    {
        float angle = static_cast<float>(i) * angleStep;
        float px = x + radius * std::cosf(angle);
        float py = y + radius * std::sinf(angle);
        _boundary.emplace_back(px, py);
    }
}

void InkDrop::marbled(const InkDrop &other)
{
    Point c = other.getInitialCentre();
    float orad = other.getInitialRadius();
    for (auto &p : _boundary)
    {
        Point disp = p - c;
        float mag = disp.length();
        if (mag < 1e-6f)
            continue;
        float scaling = std::sqrt(1.0f + (orad * orad) / (mag * mag));
        p = c + disp * scaling;
    }
}

void InkDrop::tine(const Point &m, float bx, float by, float z, float c)
{
    Point b(bx, by);
    Point n(-m.y, m.x);
    float u = 1.0f / std::pow(2.0f, 1.0f / c);
    for (auto &v : _boundary)
    {
        Point pb = v - b;
        float d = std::fabs(pb.dot(n));
        float mag = z * std::pow(u, d);
        v = v + (m * mag);
    }
}

void InkDrop::vortex(const Point &pos, float z, float c, float r)
{
    float u = 1.0f / std::pow(2.0f, 1.0f / c);
    for (auto &v : _boundary)
    {
        Point translated = v - pos;
        float h = translated.length();
        if (h == 0.0f)
            continue;
        float i_val = z * std::pow(u, h - r);
        float a = i_val / h;
        float ca = std::cos(a);
        float sa = std::sin(a);
        float rx = translated.x * ca - translated.y * sa;
        float ry = translated.x * sa + translated.y * ca;
        v = pos + Point(rx, ry);
    }
}

const std::vector<Point> &InkDrop::getBoundary() const
{
    return _boundary;
}

const Color &InkDrop::getColor() const
{
    return _color;
}

Point InkDrop::getInitialCentre() const
{
    return _initialCentre;
}

float InkDrop::getInitialRadius() const
{
    return _initialRadius;
}
