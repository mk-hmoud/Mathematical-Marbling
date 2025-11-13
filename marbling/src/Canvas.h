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

    void add_drop(float x, float y, float radius = 75.0f, const Color &color = {1, 1, 1});
    void tine(const Point &start, const Point &end, const TineSettings &settings);
    void comb(const Point &start, const Point &end, const CombSettings &settings);
    void vortex(const Point &pos, const VortexSettings &settings);
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