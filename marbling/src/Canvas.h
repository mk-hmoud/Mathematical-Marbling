#ifndef CANVAS_H
#define CANVAS_H

#include <vector>
#include "InkDrop.h"

class Canvas
{
public:
    Canvas(int width, int height);

    void add_drop(float x, float y, float radius = 75.0f);
    void tine(const Point &start, const Point &end, float z, float c);
    void vortex(float x, float y, float z, float c, float r);
    void clear();

    const std::vector<InkDrop> &getDrops() const;

private:
    float randomFloat(float lower, float upper);

private:
    int _width, _height;
    std::vector<InkDrop> _inkDrops;
};

#endif // CANVAS_H