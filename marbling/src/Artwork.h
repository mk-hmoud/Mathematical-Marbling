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

    /**
     * @brief Adds a drop at a random position. If @p palette is non-empty the
     * color is a random pick from it (harmonious); otherwise it is a random RGB.
     * The resolved values are recorded as a plain DropOp, so replay stays
     * deterministic.
     */
    void addRandomDrop(float radius = 75.0f, const std::vector<Color> &palette = {});
    void addTine(const Point &start, const Point &end, const TineSettings &settings);
    void addComb(const Point &start, const Point &end, const CombSettings &settings);
    void addVortex(const Point &pos, const VortexSettings &settings);
    void addWave(const Point &start, const Point &end, const WaveSettings &settings);
    void clear();

    /**
     * @brief Rebuilds the canvas from scratch by replaying an operation list.
     * Replaces the current history. Foundation for load and undo/redo.
     */
    void setOperations(const std::vector<Operation> &ops);

    /**
     * @brief Loads a piece: adopts a new canvas size and replays @p ops.
     */
    void load(int width, int height, const std::vector<Operation> &ops);

    // --- Nondestructive recolor ---
    // Re-skins the whole piece by remapping every drop color to its nearest
    // match in @p palette at replay time. The recorded operations keep their
    // original colors, so this is fully reversible via clearRecolor().
    void setRecolor(const std::vector<Color> &palette);
    void clearRecolor();
    bool isRecolorActive() const { return _recolorActive; }
    const std::vector<Color> &recolorPalette() const { return _recolorPalette; }

    // --- Undo / redo ---
    bool canUndo() const { return !_undoSteps.empty(); }
    bool canRedo() const { return !_redoSteps.empty(); }

    /** @brief Reverts the most recent step (one op, or one batch). */
    void undo();
    /** @brief Re-applies the most recently undone step. */
    void redo();

    // --- Operation grouping ---
    // Operations recorded between beginBatch() and endBatch() collapse into a
    // single undo/redo step, so a multi-op pattern undoes in one action.
    void beginBatch();
    void endBatch();

    // --- Live gesture preview ---
    // Shows the result of an in-progress operation without recording it, so a
    // tine/comb/vortex deforms the ink under the cursor in real time. The
    // gesture is committed by calling the matching add*() on release, then
    // endPreview() to drop the transient state.
    void previewTine(const Point &start, const Point &end, const TineSettings &settings);
    void previewComb(const Point &start, const Point &end, const CombSettings &settings);
    void previewVortex(const Point &pos, const VortexSettings &settings);
    void previewWave(const Point &start, const Point &end, const WaveSettings &settings);
    void endPreview();

    // --- Accessors ---
    const std::vector<Operation> &operations() const { return _ops; }
    int width() const { return _width; }
    int height() const { return _height; }

    // --- Rendering data ---
    std::vector<Polygon> getPolygons() const
    {
        return _previewActive ? _previewCanvas.getPolygons() : _canvas.getPolygons();
    }
    bool isDirty() const { return _dirty; }
    void clearDirty() { _dirty = false; }

private:
    void record(const Operation &op); // clear redo, apply, append to history
    void apply(const Operation &op);  // apply one op to the canvas
    void rebuild();                   // replay the whole active history
    Color mapColor(const Color &c) const; // recolor remap (identity if off)

    int _width, _height;
    std::mt19937 _rng;
    Canvas _canvas;         // committed geometry
    Canvas _previewCanvas;  // committed geometry + the in-progress gesture
    std::vector<Operation> _ops;  // active history (oldest first)
    std::vector<Operation> _redo; // undone ops (grouped, in original order)
    std::vector<size_t> _undoSteps; // op-count per undoable step; sums to _ops
    std::vector<size_t> _redoSteps; // op-count per redoable step; sums to _redo
    bool _batchActive = false;
    size_t _batchCount = 0; // ops recorded in the current batch
    bool _previewActive = false;
    bool _dirty = true;     // geometry changed since the last clearDirty()

    bool _recolorActive = false;
    std::vector<Color> _recolorPalette; // target palette for nearest-remap
};

#endif // ARTWORK_H
