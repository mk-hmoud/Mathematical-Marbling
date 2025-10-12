#ifndef RENDERER_H
#define RENDERER_H

#include "Canvas.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void drawCanvas(const Canvas &canvas);
    void drawDragLine(const Point &start, const Point &end);
    void setupProjection(int width, int height);
};

#endif // RENDERER_H