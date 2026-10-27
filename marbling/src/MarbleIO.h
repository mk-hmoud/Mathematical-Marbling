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
    /** @brief Writes the canvas size and operation list to @p path. */
    bool save(const std::string &path, int width, int height, const std::vector<Operation> &ops);

    /**
     * @brief Reads a .marble file into @p width, @p height and @p ops.
     * @return true on success; on failure the out-params are left unspecified.
     */
    bool load(const std::string &path, int &width, int &height, std::vector<Operation> &ops);
}

#endif // MARBLE_IO_H
