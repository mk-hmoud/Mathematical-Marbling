#ifndef THEME_H
#define THEME_H

#include "imgui.h"

/**
 * @brief Cohesive dark "studio" theme for the marbling app: a neutral
 * near-black grey palette with a single azure accent, consistent rounding and
 * spacing, and real typography loaded from a system TTF (with graceful
 * fallback to the built-in font). Styled to read as a finished product rather
 * than the default ImGui look.
 */
namespace Theme
{
    // Fonts loaded at startup. Any pointer may be null if no TTF was found, in
    // which case callers simply skip PushFont and get the default font.
    struct Fonts
    {
        ImFont *body = nullptr;  // default UI text
        ImFont *bold = nullptr;  // section titles, tool labels, emphasis
        ImFont *h1 = nullptr;    // large: app title / headline
        ImFont *small = nullptr; // captions, status bar
    };

    // A few shared accent colors the UI reuses for consistency.
    extern ImVec4 Accent;       // primary azure
    extern ImVec4 AccentHover;  // brighter
    extern ImVec4 AccentDim;    // low-alpha fill
    extern ImVec4 TextMuted;    // secondary text

    // Build the palette + metrics. dpiScale scales spacing/padding (not fonts).
    void Apply(float dpiScale = 1.0f);

    // Load UI fonts. dpiScale bakes the glyphs larger on HiDPI for crispness.
    // Must be called before the first frame (before the font atlas is built).
    Fonts LoadFonts(ImGuiIO &io, float dpiScale = 1.0f);
}

#endif // THEME_H
