#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "DataTypes.h"

class Renderer
{
public:
    Renderer();
    ~Renderer() = default;

    void drawPolygons(const std::vector<Polygon> &polygons);
    void drawDragLine(const Point &start, const Point &end);
    void setupProjection(int width, int height);
};

#endif // RENDERER_H