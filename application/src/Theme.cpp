#include "Theme.h"

#include <cstdio>

namespace Theme
{
    ImVec4 Accent      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImVec4 AccentHover = ImVec4(0.38f, 0.68f, 1.00f, 1.00f);
    ImVec4 AccentDim   = ImVec4(0.26f, 0.59f, 0.98f, 0.28f);
    ImVec4 TextMuted   = ImVec4(0.56f, 0.57f, 0.60f, 1.00f);

    namespace
    {
        // Pick the first readable TTF from a small set of common system faces.
        const char *firstExisting(const char *const *paths, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                if (FILE *f = std::fopen(paths[i], "rb"))
                {
                    std::fclose(f);
                    return paths[i];
                }
            }
            return nullptr;
        }
    }

    void Apply(float dpiScale)
    {
        ImGuiStyle &s = ImGui::GetStyle();
        ImGui::StyleColorsDark(&s); // sane baseline, then override below

        // --- Metrics (authored at 1x; ScaleAllSizes handles HiDPI) ---
        s.WindowPadding     = ImVec2(14.0f, 12.0f);
        s.FramePadding      = ImVec2(10.0f, 6.0f);
        s.ItemSpacing       = ImVec2(10.0f, 8.0f);
        s.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
        s.IndentSpacing     = 18.0f;
        s.ScrollbarSize     = 13.0f;
        s.GrabMinSize       = 12.0f;
        s.CellPadding       = ImVec2(8.0f, 6.0f);

        s.WindowBorderSize  = 1.0f;
        s.ChildBorderSize   = 1.0f;
        s.PopupBorderSize   = 1.0f;
        s.FrameBorderSize   = 1.0f;
        s.TabBorderSize     = 0.0f;

        s.WindowRounding    = 8.0f;
        s.ChildRounding     = 7.0f;
        s.PopupRounding     = 7.0f;
        s.FrameRounding     = 6.0f;
        s.GrabRounding      = 6.0f;
        s.ScrollbarRounding = 9.0f;
        s.TabRounding       = 6.0f;

        s.WindowTitleAlign       = ImVec2(0.0f, 0.5f);
        s.WindowMenuButtonPosition = ImGuiDir_None; // hide the dock collapse arrow
        s.SeparatorTextBorderSize = 2.0f;
        s.SeparatorTextPadding    = ImVec2(16.0f, 6.0f);
        s.DockingSeparatorSize    = 2.0f;

        // --- Palette ---
        const ImVec4 bg        = ImVec4(0.118f, 0.122f, 0.133f, 1.00f); // #1E1F22
        const ImVec4 bgChild   = ImVec4(0.102f, 0.106f, 0.118f, 1.00f); // #1A1B1E
        const ImVec4 bgPopup   = ImVec4(0.137f, 0.141f, 0.153f, 1.00f); // #232427
        const ImVec4 frame     = ImVec4(0.166f, 0.172f, 0.188f, 1.00f); // #2A2C30
        const ImVec4 frameHov  = ImVec4(0.204f, 0.212f, 0.231f, 1.00f); // #33363B
        const ImVec4 frameAct  = ImVec4(0.231f, 0.239f, 0.262f, 1.00f); // #3A3D43
        const ImVec4 button    = ImVec4(0.180f, 0.188f, 0.204f, 1.00f);
        const ImVec4 buttonHov = frameHov;
        const ImVec4 text      = ImVec4(0.902f, 0.906f, 0.914f, 1.00f);
        const ImVec4 border    = ImVec4(1.000f, 1.000f, 1.000f, 0.055f);
        const ImVec4 separator = ImVec4(1.000f, 1.000f, 1.000f, 0.090f);

        ImVec4 *c = s.Colors;
        c[ImGuiCol_Text]                 = text;
        c[ImGuiCol_TextDisabled]         = TextMuted;
        c[ImGuiCol_WindowBg]             = bg;
        c[ImGuiCol_ChildBg]              = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg]              = bgPopup;
        c[ImGuiCol_Border]               = border;
        c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg]              = frame;
        c[ImGuiCol_FrameBgHovered]       = frameHov;
        c[ImGuiCol_FrameBgActive]        = frameAct;
        c[ImGuiCol_TitleBg]              = bgChild;
        c[ImGuiCol_TitleBgActive]        = bgChild;
        c[ImGuiCol_TitleBgCollapsed]     = bgChild;
        c[ImGuiCol_MenuBarBg]            = bgChild;
        c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab]        = frameAct;
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.31f, 0.34f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive]  = Accent;
        c[ImGuiCol_CheckMark]            = AccentHover;
        c[ImGuiCol_SliderGrab]           = Accent;
        c[ImGuiCol_SliderGrabActive]     = AccentHover;
        c[ImGuiCol_Button]               = button;
        c[ImGuiCol_ButtonHovered]        = buttonHov;
        c[ImGuiCol_ButtonActive]         = frameAct;
        c[ImGuiCol_Header]               = AccentDim;
        c[ImGuiCol_HeaderHovered]        = ImVec4(Accent.x, Accent.y, Accent.z, 0.40f);
        c[ImGuiCol_HeaderActive]         = ImVec4(Accent.x, Accent.y, Accent.z, 0.55f);
        c[ImGuiCol_Separator]            = separator;
        c[ImGuiCol_SeparatorHovered]     = AccentDim;
        c[ImGuiCol_SeparatorActive]      = Accent;
        c[ImGuiCol_ResizeGrip]           = ImVec4(1, 1, 1, 0.06f);
        c[ImGuiCol_ResizeGripHovered]    = AccentDim;
        c[ImGuiCol_ResizeGripActive]     = Accent;
        c[ImGuiCol_Tab]                  = bgChild;
        c[ImGuiCol_TabHovered]           = frameHov;
        c[ImGuiCol_TabActive]            = frame;
        c[ImGuiCol_TabUnfocused]         = bgChild;
        c[ImGuiCol_TabUnfocusedActive]   = frame;
        c[ImGuiCol_DockingPreview]       = AccentDim;
        c[ImGuiCol_DockingEmptyBg]       = bgChild;
        c[ImGuiCol_PlotLines]            = Accent;
        c[ImGuiCol_PlotHistogram]        = Accent;
        c[ImGuiCol_TextSelectedBg]       = AccentDim;
        c[ImGuiCol_NavHighlight]         = Accent;
        c[ImGuiCol_DragDropTarget]       = AccentHover;

        if (dpiScale > 1.0f)
            s.ScaleAllSizes(dpiScale);
    }

    Fonts LoadFonts(ImGuiIO &io, float dpiScale)
    {
        Fonts fonts;

        // Prefer a clean, metric-compatible sans; fall back across common
        // Linux faces. These are static TTFs that stb_truetype rasterizes well.
        static const char *regular[] = {
            "/usr/share/fonts/google-carlito-fonts/Carlito-Regular.ttf",
            "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
            "/usr/share/fonts/google-droid-sans-fonts/DroidSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        };
        static const char *boldFaces[] = {
            "/usr/share/fonts/google-carlito-fonts/Carlito-Bold.ttf",
            "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        };

        const char *reg = firstExisting(regular, (int)(sizeof(regular) / sizeof(regular[0])));
        const char *bld = firstExisting(boldFaces, (int)(sizeof(boldFaces) / sizeof(boldFaces[0])));

        if (!reg)
        {
            io.Fonts->AddFontDefault();
            return fonts; // all pointers stay null -> callers use default
        }

        ImFontConfig cfg;
        cfg.OversampleH = 3;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = false;

        const float s = dpiScale > 0.0f ? dpiScale : 1.0f;
        fonts.body  = io.Fonts->AddFontFromFileTTF(reg, 16.5f * s, &cfg);
        fonts.small = io.Fonts->AddFontFromFileTTF(reg, 14.0f * s, &cfg);
        if (bld)
        {
            fonts.bold = io.Fonts->AddFontFromFileTTF(bld, 16.5f * s, &cfg);
            fonts.h1   = io.Fonts->AddFontFromFileTTF(bld, 22.0f * s, &cfg);
        }
        else
        {
            fonts.bold = fonts.body;
            fonts.h1   = io.Fonts->AddFontFromFileTTF(reg, 22.0f * s, &cfg);
        }

        io.FontDefault = fonts.body;
        return fonts;
    }
}
