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

    /**
     * @brief Adds an ink drop at a random position within [0,width]x[0,height]
     * with a random RGB color, using the canvas's own RNG.
     */
    void add_random_drop(int width, int height, float radius = 75.0f);
    void tine(const Point &start, const Point &end, const TineSettings &settings);
    void comb(const Point &start, const Point &end, const CombSettings &settings);
    void vortex(const Point &pos, const VortexSettings &settings);
    void clear();

    std::vector<Polygon> getPolygons() const;

    /**
     * @brief True when the ink geometry changed since the last clearDirty().
     * The renderer uses this to re-tessellate/re-upload only on edits.
     */
    bool isDirty() const { return _dirty; }
    void clearDirty() { _dirty = false; }

private:
    float randomFloat(float lower, float upper);

private:
    int _width, _height;
    std::vector<InkDrop> _inkDrops;
    std::mt19937 _rng;
    bool _dirty = true;
};

#endif // CANVAS_H