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
};

/**
 * @brief Holds the configuration for a vortex.
 */
struct VortexSettings
{
    float z = 200.0f;
    float c = 60.0f;
    float r = 50.0f;
};

#endif // DATATYPES_H