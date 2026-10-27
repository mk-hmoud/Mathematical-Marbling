#ifndef SVG_WRITER_H
#define SVG_WRITER_H

#include <string>
#include <vector>

#include "DataTypes.h"

/**
 * @brief Writes the polygons to an SVG file (resolution-independent vector).
 *
 * @param path       Output file path.
 * @param width      Canvas width in coordinate units (SVG viewBox).
 * @param height     Canvas height in coordinate units.
 * @param polygons   Filled polygons, in canvas coordinates.
 * @param background Background color (ignored if @p transparent).
 * @param transparent If true, no background rectangle is emitted.
 * @return true on success.
 *
 * Note: mirror symmetry is not applied to SVG output.
 */
bool writeSvg(const std::string &path, int width, int height,
              const std::vector<Polygon> &polygons,
              const Color &background, bool transparent);

#endif // SVG_WRITER_H
