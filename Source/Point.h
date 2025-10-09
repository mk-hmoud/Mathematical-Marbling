#pragma once

#include <cmath>

class Point {
public:
    float x, y;

    Point() {}
    Point(float x, float y) : x(x), y(y) {}

    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }

    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }

    Point operator*(float scalar) const {
        return Point(x * scalar, y * scalar);
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }
};