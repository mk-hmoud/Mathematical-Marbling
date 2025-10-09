#pragma once

#include <vector>
#include <GL/glew.h>
#include "Point.h"

class InkDrop {
public:
    InkDrop(float x, float y, float radius, float r=1.0f, float g=1.0f, float b=1.0f);
    ~InkDrop();
    void render() const;
    void marbled(const InkDrop& other);
    void vortex(const Point& pos, float z, float c, float r);
    void tine(const Point& m, float x, float y, float z, float c);
    float getX()const;
    float getY()const;
    float getRadius()const;
    void fill()const;
    void drawPixel(float x, float y)const;

private:

    Point centre;
    float radius;
    float r, g, b;
    std::vector<Point> vertices;
};