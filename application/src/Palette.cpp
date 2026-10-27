#include "Palette.h"

namespace
{
    // 0-255 convenience so the curated values read like hex colors.
    Color rgb(int r, int g, int b)
    {
        return Color{r / 255.0f, g / 255.0f, b / 255.0f};
    }
}

std::vector<Palette> builtinPalettes()
{
    return {
        {"Classic Marble",
         {rgb(26, 42, 74), rgb(42, 93, 122), rgb(90, 169, 169),
          rgb(233, 221, 190), rgb(176, 58, 46), rgb(30, 30, 38)}},

        {"Sunset",
         {rgb(34, 29, 56), rgb(102, 42, 94), rgb(201, 73, 95),
          rgb(237, 122, 79), rgb(248, 183, 92), rgb(252, 232, 180)}},

        {"Ocean",
         {rgb(8, 37, 66), rgb(20, 83, 112), rgb(40, 140, 150),
          rgb(80, 184, 175), rgb(160, 216, 204), rgb(235, 247, 240)}},

        {"Earth",
         {rgb(43, 33, 24), rgb(97, 71, 45), rgb(150, 110, 60),
          rgb(188, 156, 97), rgb(124, 130, 79), rgb(222, 211, 176)}},

        {"Vivid",
         {rgb(230, 57, 70), rgb(241, 143, 1), rgb(255, 209, 102),
          rgb(6, 167, 125), rgb(17, 138, 178), rgb(118, 68, 170)}},

        {"Mono",
         {rgb(18, 18, 18), rgb(70, 70, 70), rgb(122, 122, 122),
          rgb(172, 172, 172), rgb(214, 214, 214), rgb(245, 245, 245)}},
    };
}
