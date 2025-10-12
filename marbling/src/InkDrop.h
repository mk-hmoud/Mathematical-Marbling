#ifndef INKDROP_H
#define INKDROP_H

#include <vector>
#include "Point.h"

class InkDrop
{
public:
    InkDrop(float x, float y, float radius, float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // --- Math Marbling Functions ---
    void marbled(const InkDrop &other);
    void vortex(const Point &pos, float z, float c, float r);
    void tine(const Point &m, float x, float y, float z, float c);

    // --- Data access methods for the Renderer ---
    const std::vector<Point> &getVertices() const;
    void getColor(float &red, float &green, float &blue) const;
    Point getCentre() const;
    float getRadius() const;

private:
    Point _centre;
    float _radius;
    float _r, _g, _b;
    std::vector<Point> _vertices;
};

#endif // INKDROP_H