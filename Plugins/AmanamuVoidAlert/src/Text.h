// Text helpers: the plugin's texts follow the program language (PluginI18n.h, through
// PluginSDK.h). Every text a user reads is a key of lang/en.json; numbers are formatted here,
// in code, into named TrF arguments, never by a translation.
#pragma once

#include "../../../POEFixer/plugin_sdk/PluginSDK.h"
#include "TextUtil.h"

#include <sal.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>

namespace AmanamuVoid {

namespace I18n = PluginSDK::I18n;

// printf for a TrF argument: Num("%.1f", seconds). The format is code, never a translation.
inline std::string Num(_In_z_ _Printf_format_string_ const char* format, ...) {
    char buf[64];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return buf;
}

} // namespace AmanamuVoid
