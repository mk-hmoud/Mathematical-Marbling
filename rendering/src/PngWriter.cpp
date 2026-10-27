#include "PngWriter.h"

#include <cstdio>
#include <cstdint>
#include <vector>
#include <zlib.h>

namespace
{
    // Append a big-endian 32-bit value.
    void putBE32(std::vector<unsigned char> &out, uint32_t v)
    {
        out.push_back(static_cast<unsigned char>((v >> 24) & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 16) & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(v & 0xFF));
    }

    // Write one PNG chunk (length, type, data, CRC) to the file.
    bool writeChunk(FILE *f, const char *type, const unsigned char *data, uint32_t len)
    {
        unsigned char lenBE[4] = {
            static_cast<unsigned char>((len >> 24) & 0xFF),
            static_cast<unsigned char>((len >> 16) & 0xFF),
            static_cast<unsigned char>((len >> 8) & 0xFF),
            static_cast<unsigned char>(len & 0xFF)};
        if (std::fwrite(lenBE, 1, 4, f) != 4)
            return false;
        if (std::fwrite(type, 1, 4, f) != 4)
            return false;
        if (len && std::fwrite(data, 1, len, f) != len)
            return false;

        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const Bytef *>(type), 4);
        if (len)
            crc = crc32(crc, data, len);

        unsigned char crcBE[4] = {
            static_cast<unsigned char>((crc >> 24) & 0xFF),
            static_cast<unsigned char>((crc >> 16) & 0xFF),
            static_cast<unsigned char>((crc >> 8) & 0xFF),
            static_cast<unsigned char>(crc & 0xFF)};
        return std::fwrite(crcBE, 1, 4, f) == 4;
    }
}

bool writePng(const std::string &path, int width, int height, const unsigned char *rgba)
{
    if (width <= 0 || height <= 0 || rgba == nullptr)
        return false;

    const size_t rowBytes = static_cast<size_t>(width) * 4;

    // Build the raw (filtered) scanlines: each row is prefixed with a filter
    // byte of 0 ("None"), as required by the PNG spec before compression.
    std::vector<unsigned char> raw;
    raw.reserve(static_cast<size_t>(height) * (1 + rowBytes));
    for (int y = 0; y < height; ++y)
    {
        raw.push_back(0);
        const unsigned char *row = rgba + static_cast<size_t>(y) * rowBytes;
        raw.insert(raw.end(), row, row + rowBytes);
    }

    // Deflate the scanlines into the IDAT payload.
    uLongf compCap = compressBound(static_cast<uLong>(raw.size()));
    std::vector<unsigned char> comp(compCap);
    if (compress2(comp.data(), &compCap, raw.data(), static_cast<uLong>(raw.size()), Z_BEST_SPEED) != Z_OK)
        return false;
    comp.resize(compCap);

    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;

    bool ok = true;
    const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    ok = ok && std::fwrite(signature, 1, 8, f) == 8;

    // IHDR: width, height, bit depth 8, color type 6 (RGBA), default fields.
    std::vector<unsigned char> ihdr;
    putBE32(ihdr, static_cast<uint32_t>(width));
    putBE32(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8); // bit depth
    ihdr.push_back(6); // color type: truecolor with alpha
    ihdr.push_back(0); // compression
    ihdr.push_back(0); // filter
    ihdr.push_back(0); // interlace
    ok = ok && writeChunk(f, "IHDR", ihdr.data(), static_cast<uint32_t>(ihdr.size()));
    ok = ok && writeChunk(f, "IDAT", comp.data(), static_cast<uint32_t>(comp.size()));
    ok = ok && writeChunk(f, "IEND", nullptr, 0);

    std::fclose(f);
    return ok;
}
