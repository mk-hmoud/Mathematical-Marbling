#include "Canvas.h"

#include <random>

Canvas::Canvas(int width, int height)
    : _width(width), _height(height), _rng(std::random_device{}())
{
}

float Canvas::randomFloat(float lower, float upper)
{
    std::uniform_real_distribution<float> dist(lower, upper);
    return dist(_rng);
}

void Canvas::add_drop(float x, float y, float /*radius*/)
{
    float rad = randomFloat(40.0f, 100.0f);
    float rr = randomFloat(0.0f, 1.0f);
    float gg = randomFloat(0.0f, 1.0f);
    float bb = randomFloat(0.0f, 1.0f);
    InkDrop new_drop(x, y, rad, rr, gg, bb);
    for (auto &drop : _inkDrops)
    {
        drop.marbled(new_drop);
    }
    _inkDrops.push_back(std::move(new_drop));
}

void Canvas::tine(const Point &start, const Point &end, float z, float c)
{
    Point dir = (end - start).normalized();
    for (auto &drop : _inkDrops)
    {
        drop.tine(dir, start.x, start.y, z, c);
    }
}

void Canvas::vortex(float x, float y, float z, float c, float r)
{
    Point pos(x, y);
    for (auto &drop : _inkDrops)
    {
        drop.vortex(pos, z, c, r);
    }
}

void Canvas::clear()
{
    _inkDrops.clear();
}

std::vector<Polygon> Canvas::getPolygons() const
{
    std::vector<Polygon> polygons;
    polygons.reserve(_inkDrops.size());
    for (const auto &drop : _inkDrops)
    {
        polygons.push_back({drop.getBoundary(), drop.getColor()});
    }
    return polygons;
}