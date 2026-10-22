// The Amanamu Void Alert tab in the host's Plugins window. One small function per
// card, so each card can move to the shared plugin UI kit on its own.
#pragma once

#include "Overlay.h"
#include "Settings.h"
#include "Tracker.h"

namespace AmanamuVoid {

struct PanelContext {
    Settings&           settings;
    Tracker&            tracker;
    const OverlayStats& overlay;
    bool&               showDebugWindow;   // session only
    bool                hostCompatible;
};

// Returns true when a setting changed (persist once no widget is being edited).
bool DrawSettingsPanel(PanelContext& pc);

} // namespace AmanamuVoid
