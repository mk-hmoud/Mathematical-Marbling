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

#endif // DATATYPES_H