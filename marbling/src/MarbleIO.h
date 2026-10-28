#ifndef MARBLE_IO_H
#define MARBLE_IO_H

#include <string>
#include <vector>

#include "Operation.h"

/**
 * @file MarbleIO.h
 * @brief Save/load of a marbling piece as its operation history.
 *
 * The on-disk format (".marble") is a small, versioned, line-based text format.
 * Because a piece is fully described by its canvas size plus the ordered list
 * of operations, the file is tiny and the artwork re-renders identically at any
 * resolution when loaded. Floats are written with enough precision to round
 * trip exactly, so a loaded piece replays to the same geometry.
 */
namespace MarbleIO
{
    /**
     * @brief Writes the canvas size, operation list, and (if non-empty) the
     * active recolor palette to @p path.
     */
    bool save(const std::string &path, int width, int height,
              const std::vector<Operation> &ops,
              const std::vector<Color> &recolorPalette = {});

    /**
     * @brief Reads a .marble file into @p width, @p height, @p ops and (if
     * present) @p recolorPalette. @return true on success.
     */
    bool load(const std::string &path, int &width, int &height,
              std::vector<Operation> &ops, std::vector<Color> &recolorPalette);
}

#endif // MARBLE_IO_H
