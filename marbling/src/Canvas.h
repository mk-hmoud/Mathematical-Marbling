#ifndef CANVAS_H
#define CANVAS_H

#include <vector>
#include "DataTypes.h"
#include "InkDrop.h"

/**
 * @class Canvas
 * @brief Pure, deterministic marbling geometry engine.
 *
 * Holds the current ink geometry and applies marbling operations to it. It has
 * no randomness and no I/O, so a given sequence of calls always produces the
 * same result. Higher layers (Artwork) own the operation history and any
 * randomness, which keeps replay deterministic.
 */
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

    /**
     * @brief True when the ink geometry changed since the last clearDirty().
     * The renderer uses this to re-tessellate/re-upload only on edits.
     */
    bool isDirty() const { return _dirty; }
    void clearDirty() { _dirty = false; }

private:
    int _width, _height;
    std::vector<InkDrop> _inkDrops;
    bool _dirty = true;
};

#endif // CANVAS_H