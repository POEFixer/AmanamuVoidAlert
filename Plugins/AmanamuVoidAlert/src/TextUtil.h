// Text helpers that need no SDK (STL only), so the tests build them: a slider's display
// format from a translated unit text.
#pragma once

#include <string>
#include <string_view>

namespace AmanamuVoid {

// The unit text ("{value} s") with its FIRST {value} replaced by the number format ("%.0f").
// The number format is code; a '%' of the translation is doubled so it is never a conversion,
// and a later {value} stays literal text: ImGui passes one number, so a second conversion
// would read an argument that is not there.
inline std::string UnitFormat(const char* unitText, const char* numberFormat) {
    const std::string_view text = unitText ? unitText : "";
    const std::string_view mark = "{value}";
    std::string out;
    bool placed = false;
    for (size_t i = 0; i < text.size();) {
        if (!placed && text.compare(i, mark.size(), mark) == 0) {
            out += numberFormat ? numberFormat : "";
            placed = true;
            i += mark.size();
            continue;
        }
        if (text[i] == '%') out += '%';
        out += text[i++];
    }
    return out;
}

} // namespace AmanamuVoid
