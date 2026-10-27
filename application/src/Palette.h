#ifndef PALETTE_H
#define PALETTE_H

#include <string>
#include <vector>

#include "DataTypes.h"

/**
 * @struct Palette
 * @brief A named set of colors the artist picks from, instead of raw RGB.
 */
struct Palette
{
    std::string name;
    std::vector<Color> colors;
};

/**
 * @brief Returns the built-in curated palettes. Always non-empty, and every
 * palette has at least one color.
 */
std::vector<Palette> builtinPalettes();

#endif // PALETTE_H
