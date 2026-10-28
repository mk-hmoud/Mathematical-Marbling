#include "MarbleIO.h"

#include <fstream>
#include <sstream>
#include <limits>
#include <variant>

namespace
{
    constexpr int FORMAT_VERSION = 1;

    // Enough significant digits to round-trip a 32-bit float exactly.
    constexpr int FLOAT_DIGITS = std::numeric_limits<float>::max_digits10;
}

bool MarbleIO::save(const std::string &path, int width, int height,
                    const std::vector<Operation> &ops,
                    const std::vector<Color> &recolorPalette)
{
    std::ofstream out(path);
    if (!out)
        return false;

    out.precision(FLOAT_DIGITS);
    out << "MARBLE " << FORMAT_VERSION << "\n";
    out << "canvas " << width << " " << height << "\n";

    if (!recolorPalette.empty())
    {
        out << "recolor " << recolorPalette.size();
        for (const Color &c : recolorPalette)
            out << ' ' << c.r << ' ' << c.g << ' ' << c.b;
        out << "\n";
    }

    for (const auto &op : ops)
    {
        std::visit(
            [&out](const auto &o)
            {
                using T = std::decay_t<decltype(o)>;
                if constexpr (std::is_same_v<T, DropOp>)
                    out << "drop " << o.center.x << ' ' << o.center.y << ' ' << o.radius
                        << ' ' << o.color.r << ' ' << o.color.g << ' ' << o.color.b << '\n';
                else if constexpr (std::is_same_v<T, TineOp>)
                    out << "tine " << o.start.x << ' ' << o.start.y << ' ' << o.end.x << ' ' << o.end.y
                        << ' ' << o.settings.z << ' ' << o.settings.c << '\n';
                else if constexpr (std::is_same_v<T, CombOp>)
                    out << "comb " << o.start.x << ' ' << o.start.y << ' ' << o.end.x << ' ' << o.end.y
                        << ' ' << o.settings.numTines << ' ' << o.settings.spacing
                        << ' ' << o.settings.properties.z << ' ' << o.settings.properties.c << '\n';
                else if constexpr (std::is_same_v<T, VortexOp>)
                    out << "vortex " << o.pos.x << ' ' << o.pos.y
                        << ' ' << o.settings.z << ' ' << o.settings.c << ' ' << o.settings.r << '\n';
                else if constexpr (std::is_same_v<T, WaveOp>)
                    out << "wave " << o.start.x << ' ' << o.start.y << ' ' << o.end.x << ' ' << o.end.y
                        << ' ' << o.settings.amplitude << ' ' << o.settings.wavelength << '\n';
                else if constexpr (std::is_same_v<T, ClearOp>)
                    out << "clear\n";
            },
            op);
    }

    return static_cast<bool>(out);
}

bool MarbleIO::load(const std::string &path, int &width, int &height,
                    std::vector<Operation> &ops, std::vector<Color> &recolorPalette)
{
    recolorPalette.clear();
    std::ifstream in(path);
    if (!in)
        return false;

    std::string line;
    if (!std::getline(in, line))
        return false;

    // Header: "MARBLE <version>".
    {
        std::istringstream header(line);
        std::string magic;
        int version = 0;
        if (!(header >> magic >> version) || magic != "MARBLE" || version > FORMAT_VERSION)
            return false;
    }

    ops.clear();
    bool haveCanvas = false;

    while (std::getline(in, line))
    {
        std::istringstream ss(line);
        std::string tag;
        if (!(ss >> tag) || tag.empty() || tag[0] == '#')
            continue; // blank line or comment

        if (tag == "canvas")
        {
            if (!(ss >> width >> height))
                return false;
            haveCanvas = true;
        }
        else if (tag == "recolor")
        {
            size_t count = 0;
            if (!(ss >> count))
                return false;
            recolorPalette.clear();
            for (size_t i = 0; i < count; ++i)
            {
                Color c;
                if (!(ss >> c.r >> c.g >> c.b))
                    return false;
                recolorPalette.push_back(c);
            }
        }
        else if (tag == "drop")
        {
            DropOp o;
            if (!(ss >> o.center.x >> o.center.y >> o.radius >> o.color.r >> o.color.g >> o.color.b))
                return false;
            ops.push_back(o);
        }
        else if (tag == "tine")
        {
            TineOp o;
            if (!(ss >> o.start.x >> o.start.y >> o.end.x >> o.end.y >> o.settings.z >> o.settings.c))
                return false;
            ops.push_back(o);
        }
        else if (tag == "comb")
        {
            CombOp o;
            if (!(ss >> o.start.x >> o.start.y >> o.end.x >> o.end.y >> o.settings.numTines >> o.settings.spacing >> o.settings.properties.z >> o.settings.properties.c))
                return false;
            ops.push_back(o);
        }
        else if (tag == "vortex")
        {
            VortexOp o;
            if (!(ss >> o.pos.x >> o.pos.y >> o.settings.z >> o.settings.c >> o.settings.r))
                return false;
            ops.push_back(o);
        }
        else if (tag == "wave")
        {
            WaveOp o;
            if (!(ss >> o.start.x >> o.start.y >> o.end.x >> o.end.y >> o.settings.amplitude >> o.settings.wavelength))
                return false;
            ops.push_back(o);
        }
        else if (tag == "clear")
        {
            ops.push_back(ClearOp{});
        }
        // Unknown tags are ignored for forward compatibility.
    }

    return haveCanvas;
}
