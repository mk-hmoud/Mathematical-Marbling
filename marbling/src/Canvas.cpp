#include "Canvas.h"

// Below this many drops the thread-spawn overhead outweighs the work, so the
// per-drop loops run serially.
static constexpr int PARALLEL_THRESHOLD = 128;

// Longest boundary edge (canvas px) tolerated after an operation. Operations
// stretch the ink; re-sampling to this keeps filaments smooth instead of
// collapsing into straight chords. Re-sampling also decimates near-collinear
// points, so straight runs stay cheap while curves keep their detail.
static constexpr float MAX_EDGE = 4.0f;

Canvas::Canvas(int width, int height)
    : _width(width), _height(height)
{
}

void Canvas::add_drop(float x, float y, float radius, const Color &color)
{
    InkDrop new_drop(x, y, radius, color.r, color.g, color.b);
    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].marbled(new_drop);
        _inkDrops[i].resample(MAX_EDGE);
    }
    _inkDrops.push_back(new_drop);
    _dirty = true;
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
        _inkDrops[i].resample(MAX_EDGE);
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

    // For each drop, apply every comb tooth, then re-sample once. (Teeth act
    // only on their own drop, so iterating drop-outer is equivalent to the
    // original tooth-outer order but lets us re-sample a single time per drop.)
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        for (int t = 0; t < settings.numTines; ++t)
        {
            const Point tineStart =
                combHandleCenter - offset + perpendicular * (static_cast<float>(t) * settings.spacing);
            _inkDrops[i].tine(direction, tineStart.x, tineStart.y,
                              settings.properties.z, settings.properties.c);
        }
        _inkDrops[i].resample(MAX_EDGE);
    }
    _dirty = true;
}

void Canvas::vortex(const Point &pos, const VortexSettings &settings)
{
    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].vortex(pos, settings.strength, settings.coreRadius);
        _inkDrops[i].resample(MAX_EDGE);
    }
    _dirty = true;
}

void Canvas::wave(const Point &start, const Point &end, const WaveSettings &settings)
{
    Point direction = end - start;
    if (direction.length() < 1e-6f)
        return;
    direction.normalize();

    const int n = static_cast<int>(_inkDrops.size());
#pragma omp parallel for schedule(static) if (n > PARALLEL_THRESHOLD)
    for (int i = 0; i < n; ++i)
    {
        _inkDrops[i].wave(direction, start.x, start.y, settings.amplitude, settings.wavelength);
        _inkDrops[i].resample(MAX_EDGE);
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