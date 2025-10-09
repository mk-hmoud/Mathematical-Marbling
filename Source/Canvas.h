#pragma once

#include <vector>
#include "InkDrop.h"

class Canvas {
public:
    Canvas(int width, int height);
    void render();
    void add_drop(float x, float y, float radius=75.0f);
    void tine(float tine_x, float tine_y, float z, float c);
    void vortex(float x, float y, float z, float c, float r);
    void addDropsWithPerlinNoise(int numDrops, float scale, float z, float c);
    void clear();

private:
    float randomFloat(float lower, float upper);

    int width, height;
    std::vector<InkDrop> inkDrops;
};
