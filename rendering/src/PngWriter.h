#ifndef PNG_WRITER_H
#define PNG_WRITER_H

#include <string>

/**
 * @brief Writes an 8-bit RGBA image to a PNG file.
 *
 * @param path   Output file path.
 * @param width  Image width in pixels.
 * @param height Image height in pixels.
 * @param rgba   Tightly packed RGBA pixels, top row first, size width*height*4.
 * @return true on success, false if the file could not be written/encoded.
 */
bool writePng(const std::string &path, int width, int height, const unsigned char *rgba);

#endif // PNG_WRITER_H
