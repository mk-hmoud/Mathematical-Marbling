#ifndef ARTWORK_H
#define ARTWORK_H

#include <vector>
#include <random>

#include "Canvas.h"
#include "Operation.h"

/**
 * @class Artwork
 * @brief A marbling document: the recorded operation history plus the canvas
 *        geometry that history produces.
 *
 * Every edit is appended to the operation history and applied incrementally to
 * the canvas, so normal editing stays cheap. Because the history is the source
 * of truth, the whole piece can be saved, loaded, replayed, re-rendered at any
 * resolution, and (later) stepped through for undo/redo.
 *
 * This class is free of any windowing/UI/GL dependency so it can be reused by a
 * future non-desktop front end.
 */
class Artwork
{
public:
    Artwork(int width, int height);

    // --- Editing: records an operation and applies it to the canvas ---
    void addDrop(const Point &center, float radius, const Color &color);
    void addRandomDrop(float radius = 75.0f);
    void addTine(const Point &start, const Point &end, const TineSettings &settings);
    void addComb(const Point &start, const Point &end, const CombSettings &settings);
    void addVortex(const Point &pos, const VortexSettings &settings);
    void clear();

    /**
     * @brief Rebuilds the canvas from scratch by replaying an operation list.
     * Replaces the current history. Foundation for load and undo/redo.
     */
    void setOperations(const std::vector<Operation> &ops);

    // --- Accessors ---
    const std::vector<Operation> &operations() const { return _ops; }
    int width() const { return _width; }
    int height() const { return _height; }

    // --- Rendering data (delegated to the canvas) ---
    std::vector<Polygon> getPolygons() const { return _canvas.getPolygons(); }
    bool isDirty() const { return _canvas.isDirty(); }
    void clearDirty() { _canvas.clearDirty(); }

private:
    void apply(const Operation &op);

    int _width, _height;
    std::mt19937 _rng;
    Canvas _canvas;
    std::vector<Operation> _ops;
};

#endif // ARTWORK_H
