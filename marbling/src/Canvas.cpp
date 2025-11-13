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

void Canvas::add_drop(float x, float y, float radius, const Color &color)
{
    InkDrop new_drop(x, y, radius, color.r, color.g, color.b);
    for (auto &drop : _inkDrops)
    {
        drop.marbled(new_drop);
    }
    _inkDrops.push_back(new_drop);
}

void Canvas::tine(const Point &start, const Point &end, const TineSettings &settings)
{
    Point direction = end - start;
    if (direction.length() < 1e-6f)
        return;
    direction.normalize();

    for (auto &drop : _inkDrops)
    {
        drop.tine(direction, start.x, start.y, settings.z, settings.c);
    }
}

void Canvas::comb(const Point &start, const Point &end, const CombSettings &settings)
{
    Point direction = end - start;
    float length = direction.length();
    if (length < 1e-6f)
        return;
    direction.normalize();

    // Calculate the perpendicular vector for spacing the tines
    Point perpendicular(-direction.y, direction.x);

    // 1. Find the "center" of the comb handle (midpoint of the drag line)
    Point combHandleCenter = start + direction * (length / 2.0f);

    // 2. Find the total width of the comb
    float totalCombWidth = settings.spacing * (static_cast<float>(settings.numTines) - 1.0f);

    // 3. Find the starting offset to center the comb
    Point offset = perpendicular * (totalCombWidth / 2.0f);

    // Apply each tine ("tooth" of the comb) one by one
    for (int i = 0; i < settings.numTines; ++i)
    {
        // 4. Calculate the starting point for this specific tine
        Point tineStart = combHandleCenter - offset + perpendicular * (static_cast<float>(i) * settings.spacing);

        // 5. Apply this single tine to all drops
        for (auto &drop : _inkDrops)
        {
            drop.tine(direction, tineStart.x, tineStart.y, settings.properties.z, settings.properties.c);
        }
    }
}

void Canvas::vortex(const Point &pos, const VortexSettings &settings)
{
    for (auto &drop : _inkDrops)
    {
        drop.vortex(pos, settings.z, settings.c, settings.r);
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