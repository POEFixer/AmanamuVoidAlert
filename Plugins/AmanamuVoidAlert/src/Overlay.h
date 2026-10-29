// In-game drawing: a ring and a two-line label on each tracked monster, and an
// arrow at the screen edge pointing to monsters that are off screen. Sizes follow
// the font size and the Scale setting; colours say whether the monster is still
// in its cloud.
#pragma once

#include "../../../POEFixer/plugin_sdk/PluginSDK.h"
#include "Settings.h"
#include "Tracker.h"

#include <string>

namespace AmanamuVoid {

struct OverlayStats {
    int    drawn = 0;
    double lastFrameUs = 0.0;
};

void DrawOverlay(const PluginSDK::Context& ctx, const Settings& s, Tracker& tracker, OverlayStats& stats);

// Diagnostics window listing what is tracked; `open` is cleared by its close button.
void DrawDebugWindow(const Tracker& tracker, bool* open, double now);

// "Last pass: ..." in the program language (the debug window and the Diagnostics card).
std::string LastPassText(const TrackerStats& st);

} // namespace AmanamuVoid
