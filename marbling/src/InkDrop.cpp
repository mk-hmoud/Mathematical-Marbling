#include "InkDrop.h"

#include <cmath>
#include <array>
#include <algorithm>

namespace
{
    constexpr int MIN_VERTICES = 64;
    constexpr int MAX_VERTICES = 384;

    // Hard ceiling on a single drop's boundary so a heavily stretched filament
    // cannot grow without bound as it is re-sampled across many operations.
    constexpr int MAX_BOUNDARY = 12000;

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

void InkDrop::wave(const Point &m, float bx, float by, float amplitude, float wavelength)
{
    if (wavelength < 1e-3f)
        return;
    Point b(bx, by);
    Point n(-m.y, m.x); // axis along which the phase advances
    const float k = 2.0f * static_cast<float>(M_PI) / wavelength;
    for (auto &v : _boundary)
    {
        const float t = (v - b).dot(n);
        v = v + m * (amplitude * std::sin(k * t));
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

void InkDrop::resample(float maxEdgeLen)
{
    const int n0 = static_cast<int>(_boundary.size());
    if (n0 < 3 || maxEdgeLen <= 0.0f)
        return;

    const float maxLen2 = maxEdgeLen * maxEdgeLen;

    // --- Pass 1: subdivide edges stretched beyond maxEdgeLen -----------------
    // Marbling stretches the boundary into filaments; without inserting points
    // the fixed vertex set spreads apart and curves collapse into chords.
    bool anyLong = false;
    for (int i = 0; i < n0; ++i)
    {
        const Point d = _boundary[(i + 1) % n0] - _boundary[i];
        if (d.x * d.x + d.y * d.y > maxLen2)
        {
            anyLong = true;
            break;
        }
    }

    if (anyLong)
    {
        std::vector<Point> dense;
        dense.reserve(static_cast<size_t>(n0) + static_cast<size_t>(n0) / 2);
        for (int i = 0; i < n0; ++i)
        {
            const Point &a = _boundary[i];
            dense.push_back(a); // every original vertex is preserved

            // Once the cap is reached, stop inserting but keep copying the rest
            // of the original points so the shape is never truncated.
            if (static_cast<int>(dense.size()) >= MAX_BOUNDARY)
                continue;

            const Point &b = _boundary[(i + 1) % n0]; // closed loop
            const Point d = b - a;
            const float len2 = d.x * d.x + d.y * d.y;
            if (len2 <= maxLen2)
                continue;

            int segs = static_cast<int>(std::ceil(std::sqrt(len2) / maxEdgeLen));
            const int room = MAX_BOUNDARY - static_cast<int>(dense.size());
            if (segs - 1 > room)
                segs = room + 1;

            for (int s = 1; s < segs; ++s)
            {
                const float t = static_cast<float>(s) / static_cast<float>(segs);
                dense.emplace_back(a.x + d.x * t, a.y + d.y * t);
            }
        }
        _boundary.swap(dense);
    }

    // --- Pass 2: decimate near-collinear points ------------------------------
    // Straight stretches (e.g. the long sides of a combed ribbon) carry points
    // that add no shape; curves do not. Dropping a vertex only when it barely
    // deviates from the chord of its neighbours sheds the former while keeping
    // the latter, which is what bounds vertex growth across many operations.
    const int n = static_cast<int>(_boundary.size());
    if (n < 16)
        return; // leave small boundaries alone so drops never collapse

    const float eps = 0.3f; // max deviation from the neighbour chord (canvas px)

    std::vector<Point> out;
    out.reserve(n);
    out.push_back(_boundary[0]);
    for (int i = 1; i < n; ++i)
    {
        const Point &a = out.back();              // last kept point
        const Point &b = _boundary[i];            // candidate
        const Point &c = _boundary[(i + 1) % n];  // next

        const Point ab = b - a;
        const Point ac = c - a;
        const float acLen = ac.length();

        bool keep = true;
        if (acLen > 1e-6f)
        {
            const float dev = std::fabs(ac.x * ab.y - ac.y * ab.x) / acLen; // |ac x ab|/|ac|
            // Drop b when it is near-collinear and the merged edge a->b would
            // not itself grow past the edge target (keeps straight runs coarse
            // but never creates a long gap on a curve).
            if (dev < eps && (ab.x * ab.x + ab.y * ab.y) < maxLen2)
                keep = false;
        }
        if (keep)
            out.push_back(b);
    }

    if (static_cast<int>(out.size()) >= 8)
        _boundary.swap(out);
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
