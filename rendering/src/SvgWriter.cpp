#include "SvgWriter.h"

#include <cstdio>
#include <algorithm>

namespace
{
    int toByte(float c)
    {
        int v = static_cast<int>(c * 255.0f + 0.5f);
        return std::min(255, std::max(0, v));
    }
}

bool writeSvg(const std::string &path, int width, int height,
              const std::vector<Polygon> &polygons,
              const Color &background, bool transparent)
{
    if (width <= 0 || height <= 0)
        return false;

    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;

    std::fprintf(f,
                 "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" "
                 "viewBox=\"0 0 %d %d\">\n",
                 width, height, width, height);

    if (!transparent)
        std::fprintf(f, "  <rect width=\"%d\" height=\"%d\" fill=\"#%02x%02x%02x\"/>\n",
                     width, height, toByte(background.r), toByte(background.g), toByte(background.b));

    for (const auto &poly : polygons)
    {
        if (poly.vertices.empty())
            continue;
        std::fprintf(f, "  <polygon fill=\"#%02x%02x%02x\" points=\"",
                     toByte(poly.color.r), toByte(poly.color.g), toByte(poly.color.b));
        for (const auto &v : poly.vertices)
            std::fprintf(f, "%.2f,%.2f ", v.x, v.y);
        std::fprintf(f, "\"/>\n");
    }

    std::fprintf(f, "</svg>\n");
    const bool ok = (std::ferror(f) == 0);
    std::fclose(f);
    return ok;
}
