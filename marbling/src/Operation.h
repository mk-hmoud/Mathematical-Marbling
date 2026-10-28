#ifndef OPERATION_H
#define OPERATION_H

#include <variant>
#include "DataTypes.h"

/**
 * @file Operation.h
 * @brief The marbling operation history types.
 *
 * An artwork is a deterministic sequence of these operations applied to an
 * empty canvas. Each operation is plain, self-contained data (resolved values
 * only), so a history can be replayed to reproduce a piece
 * exactly, serialized to disk, re-rendered at any resolution, and stepped
 * through for undo/redo. Randomness is resolved to concrete values *before* an
 * operation is recorded, which is what keeps replay reproducible.
 */

/** @brief Place a single ink drop. */
struct DropOp
{
    Point center;
    float radius;
    Color color;
};

/** @brief Drag a single tine from start to end. */
struct TineOp
{
    Point start;
    Point end;
    TineSettings settings;
};

/** @brief Drag a comb (multiple tines) from start to end. */
struct CombOp
{
    Point start;
    Point end;
    CombSettings settings;
};

/** @brief Apply a vortex at a point. */
struct VortexOp
{
    Point pos;
    VortexSettings settings;
};

/** @brief Drag a wavy comb (sine shear) from start to end. */
struct WaveOp
{
    Point start;
    Point end;
    WaveSettings settings;
};

/** @brief Wipe the canvas. Kept in the history so it can be undone. */
struct ClearOp
{
};

using Operation = std::variant<DropOp, TineOp, CombOp, VortexOp, WaveOp, ClearOp>;

#endif // OPERATION_H
