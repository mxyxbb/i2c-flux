#include "imgui.h"
#include "../../fonts/font_wqdkwm.h"

void LoadFont(void)
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_cfg;

    font_cfg.FontDataOwnedByAtlas = false;

    ImFont* fontdata = io.Fonts->AddFontFromMemoryTTF(
        (void*)font_data_data,
        font_data_size,
        16.0f,
        &font_cfg,
        io.Fonts->GetGlyphRangesChineseFull()
    );
}
