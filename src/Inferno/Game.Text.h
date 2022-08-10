#pragma once

#include "Fonts.h"

namespace Inferno {
    class RenderTarget;

    extern FontAtlas Atlas;

    constexpr float FONT_LINE_SPACING = 1.2f;

    enum class AlignH { Left, Center, CenterLeft, CenterRight, Right };
    enum class AlignV { Top, Center, CenterTop, CenterBottom, Bottom };
    Vector2 MeasureString(string_view str, FontSize size);
    
    void LoadFonts();
}