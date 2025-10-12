#include "Canvas.h"

#include <random>

Canvas::Canvas(int width, int height) : _width(width), _height(height) {}

float Canvas::randomFloat(float lower, float upper)
{
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return lower + random * (upper - lower);
}

void Canvas::add_drop(float x, float y, float radius)
{
    InkDrop new_drop(x, y, randomFloat(40, 100), randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1));
    for (auto &drop : _inkDrops)
    {
        drop.marbled(new_drop);
    }
    _inkDrops.push_back(new_drop);
}

void Canvas::tine(const Point &start, const Point &end, float z, float c)
{
    Point direction = end - start;
    float length = direction.length();

    if (length > 0.0f)
    {
        direction = direction * (1.0f / length);

        for (auto &drop : _inkDrops)
        {
            drop.tine(direction, start.x, start.y, z, c);
        }
    }
}

void Canvas::vortex(float x, float y, float z, float c, float r)
{
    for (auto &drop : _inkDrops)
    {
        drop.vortex(Point(x, y), z, c, r);
    }
}

void Canvas::clear()
{
    _inkDrops.clear();
}

const std::vector<InkDrop> &Canvas::getDrops() const
{
    return _inkDrops;
}
