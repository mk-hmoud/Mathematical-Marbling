#include "Canvas.h"

#include <random>

// Below this many drops the thread-spawn overhead outweighs the work, so the
// per-drop loops run serially.
static constexpr int PARALLEL_THRESHOLD = 128;

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
    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].marbled(new_drop);
    }
    _inkDrops.push_back(new_drop);
    _dirty = true;
}

void Canvas::add_random_drop(int width, int height, float radius)
{
    float x = randomFloat(0.0f, static_cast<float>(width));
    float y = randomFloat(0.0f, static_cast<float>(height));
    Color color{randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f)};
    add_drop(x, y, radius, color);
}

void Canvas::tine(const Point &start, const Point &end, const TineSettings &settings)
{
    Point direction = end - start;
    if (direction.length() < 1e-6f)
        return;
    direction.normalize();

    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].tine(direction, start.x, start.y, settings.z, settings.c);
    }
    _dirty = true;
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

    const int n = static_cast<int>(_inkDrops.size());

    // Apply each tine ("tooth" of the comb) one by one
    for (int t = 0; t < settings.numTines; ++t)
    {
        // 4. Calculate the starting point for this specific tine
        Point tineStart = combHandleCenter - offset + perpendicular * (static_cast<float>(t) * settings.spacing);

        // 5. Apply this single tine to all drops
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
        for (int i = 0; i < n; ++i)
        {
            _inkDrops[i].tine(direction, tineStart.x, tineStart.y, settings.properties.z, settings.properties.c);
        }
    }
    _dirty = true;
}

void Canvas::vortex(const Point &pos, const VortexSettings &settings)
{
    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].vortex(pos, settings.z, settings.c, settings.r);
    }
    _dirty = true;
}

void Canvas::clear()
{
    _inkDrops.clear();
    _dirty = true;
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