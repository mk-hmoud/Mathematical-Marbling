#ifndef INKDROP_H
#define INKDROP_H

#include <vector>
#include "DataTypes.h"

class InkDrop
{
public:
    InkDrop(float x, float y, float radius, float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // --- Math Marbling Functions ---
    void marbled(const InkDrop &other);
    void vortex(const Point &pos, float z, float c, float r);
    void tine(const Point &m, float x, float y, float z, float c);

    // --- Data access methods for the Renderer ---
    const std::vector<Point> &getBoundary() const;
    const Color &getColor() const;
    Point getInitialCentre() const;
    float getInitialRadius() const;

private:
    Point _initialCentre;
    float _initialRadius;
    Color _color;
    std::vector<Point> _boundary;
};

#endif // INKDROP_H