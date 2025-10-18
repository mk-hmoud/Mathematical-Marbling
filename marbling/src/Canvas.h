#ifndef CANVAS_H
#define CANVAS_H

#include <vector>
#include "DataTypes.h"
#include "InkDrop.h"
#include <bits/stdc++.h>

class Canvas
{
public:
    Canvas(int width, int height);

    void add_drop(float x, float y, float radius = 75.0f);
    void tine(const Point &start, const Point &end, float z, float c);
    void vortex(float x, float y, float z, float c, float r);
    void clear();

    std::vector<Polygon> getPolygons() const;

private:
    float randomFloat(float lower, float upper);

private:
    int _width, _height;
    std::vector<InkDrop> _inkDrops;
    std::mt19937 _rng;
};

#endif // CANVAS_H