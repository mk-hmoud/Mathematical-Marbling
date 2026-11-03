#ifndef DATATYPES_H
#define DATATYPES_H

#include <cmath>
#include <vector>

class Point
{
public:
    float x, y;

    Point() : x(0.0f), y(0.0f) {}
    Point(float x, float y) : x(x), y(y) {}

    Point operator+(const Point &other) const
    {
        return Point(x + other.x, y + other.y);
    }

    Point operator-(const Point &other) const
    {
        return Point(x - other.x, y - other.y);
    }

    Point operator*(float scalar) const
    {
        return Point(x * scalar, y * scalar);
    }

    float dot(const Point &other) const
    {
        return x * other.x + y * other.y;
    }

    float length() const
    {
        return std::sqrt(x * x + y * y);
    }

    void normalize()
    {
        float l = length();
        if (l > 1e-6f)
        {
            x /= l;
            y /= l;
        }
    }

    Point normalized() const
    {
        float l = length();
        if (l > 0.0f)
        {
            return *this * (1.0f / l);
        }
        return Point(0.0f, 0.0f);
    }
};

struct Color
{
    float r, g, b;
};

/**
 * @brief Mirror-symmetry mode applied when drawing/exporting the canvas.
 */
enum class Symmetry
{
    None,
    MirrorX, // left/right reflection about the vertical center
    MirrorY, // top/bottom reflection about the horizontal center
    Quad,    // reflection about both axes (4-fold)
    Radial   // k-fold rotational (kaleidoscope), see fold count
};

struct Polygon
{
    std::vector<Point> vertices;
    Color color;
};

struct Triangle
{
    Point v0, v1, v2;
};

/**
 * @brief Holds the physical properties of a single tine.
 */
struct TineSettings
{
    /**
     * @brief The magnitude or strength of the pull (z-value).
     */
    float z = 80.0f;
    /**
     * @brief The decay constant or falloff (c-value).
     */
    float c = 16.0f;
};

/**
 * @brief Configuration for a finite, localized "short stroke" (Oseen flow).
 * Unlike a tine (an infinite line), the deformation is a local blob that decays
 * in every direction with the falloff length @ref spread, producing the
 * compression-ahead / V-wake-behind of a real stylus poke.
 */
struct StrokeSettings
{
    float spread = 45.0f;  // Oseen falloff length L (px): larger = broader, softer
    float strength = 1.0f; // push multiplier: core fluid moves strength * drag length
    int steps = 6;         // flow-integration sub-steps (higher = more area-preserving)
};

/**
 * @brief Holds the configuration for a "comb" (multiple tines).
 */
struct CombSettings
{
    /**
     * @brief The number of teeth on the comb.
     */
    int numTines = 10;
    /**
     * @brief The distance (in pixels) between each tine.
     */
    float spacing = 50.0f;
    /**
     * @brief The shared physical properties for all tines in this comb.
     */
    TineSettings properties;
    /**
     * @brief When true, each tooth is a finite localized Oseen stroke
     * (see @ref stroke) rather than an infinite tine line.
     */
    bool localized = false;
    StrokeSettings stroke;
};

/**
 * @brief Holds the configuration for a vortex.
 */
// Lamb-Oseen vortex: a physically-based swirl whose rotation is maximal at the
// centre and decays smoothly outward with no singularity (unlike a point
// vortex, nothing blows up near the centre).
struct VortexSettings
{
    float strength = 1.6f;    // peak rotation at the core, radians; sign sets spin direction
    float coreRadius = 55.0f; // core radius r_c (px): where the swirl is tightest
};

/**
 * @brief Holds the configuration for a wavy comb (sine shear): ink is displaced
 * along the drag direction by amplitude*sin(2*pi*t/wavelength), where t runs
 * along the axis perpendicular to the drag.
 */
struct WaveSettings
{
    float amplitude = 25.0f;
    float wavelength = 120.0f;
};

#endif // DATATYPES_H