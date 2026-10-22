#include "SettingsUi.h"

#include "../../../POEFixer/plugin_sdk/PluginUi.h"
#include "../../../POEFixer/imgui/IconsFontAwesome6.h"

#include <cmath>
#include <cstdio>

namespace AmanamuVoid {

namespace {

using PluginUi::Tip;

// A 0..1 value edited as a whole percentage (the kit's slider rows edit the raw value).
bool PercentSlider(const char* id, float& v, int minPct, int maxPct) {
    int pct = static_cast<int>(std::lround(v * 100.0f));
    if (!ImGui::SliderInt(id, &pct, minPct, maxPct, "%d%%", ImGuiSliderFlags_AlwaysClamp)) return false;
    v = static_cast<float>(pct) / 100.0f;
    return true;
}

// Swatch row with a sample of the in-game label: the overlay's outlined text on
// a small dark in-game panel, so the colour reads the same on light and dark
// themes as it will over the game.
bool ColorRow(const PluginUi::LabelColumn& col, const char* label, const char* id, Color4& c, const char* tip) {
    col.RowNoWidth(label);
    ImVec4 v(c.r, c.g, c.b, c.a);
    const bool changed = PluginUi::Swatch(id, v);
    Tip(tip);
    if (changed) c = Color4{ v.x, v.y, v.z, v.w };
    ImGui::SameLine();
    const char* sample = "AMANAMU VOID";
    const ImVec2 text = ImGui::CalcTextSize(sample);
    const float padX = ImGui::GetStyle().FramePadding.x;
    const ImVec2 size(text.x + padX * 2.0f, ImGui::GetFrameHeight());
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    if (ImGui::IsItemVisible()) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        PluginUi::Panel(dl, pos, ImVec2(pos.x + size.x, pos.y + size.y), 0.9f);
        PluginUi::TextOutlined(dl, ImVec2(pos.x + padX, pos.y + (size.y - text.y) * 0.5f),
                               ImGui::ColorConvertFloat4ToU32(v), sample);
    }
    return changed;
}

bool DrawSectionDetection(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_detect");
    PluginUi::Heading(ICON_FA_CROSSHAIRS, "Detection",
        "A monster is tracked when it has the Lightless faction monster modifier (the Amanamu abyss monster "
        "that sits in a void cloud).\n\n"
        "Its colour shows whether it still stands in its cloud: while inside it carries the "
        "abyss_lightless_well_immune buff.");

    changed |= PluginUi::Check("Only rare and unique monsters", "av_rare", &s.onlyRareOrUnique,
        "Ignore normal and magic monsters.\n"
        "Leave it on unless you see an Amanamu monster that is not marked.");
    changed |= PluginUi::Check("Also find by Lightless Well buff", "av_bybuff", &s.detectByBuff,
        "Also track monsters that carry any Lightless Well buff, even when their modifier is not readable.\n"
        "Turn it off if other monsters standing in the cloud get marked too.");

    ImGui::Spacing();
    const PluginUi::LabelColumn col{ "Tracking range", "Keep when out of range", "Forget when gone" };
    changed |= PluginUi::SliderFloatRow(col, "Tracking range", "###av_range", &s.maxDistance,
        Limits::kDistanceMin, Limits::kDistanceMax, "%.0f",
        "How far from your character (in game distance units) a monster is still picked up.");
    changed |= PluginUi::SliderFloatRow(col, "Keep when out of range", "###av_keep", &s.keepOutOfRangeSeconds,
        Limits::kKeepMin, Limits::kKeepMax, "%.0f s",
        "How long a tracked monster stays marked after it moves out of the tracking range.");
    changed |= PluginUi::SliderFloatRow(col, "Forget when gone", "###av_vanish", &s.forgetVanishedSeconds,
        Limits::kVanishMin, Limits::kVanishMax, "%.2f s",
        "How long the marker stays at the last known spot after the game stops sending the monster\n"
        "(it died or is too far away).");
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionVisibility(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_when");
    PluginUi::Heading(ICON_FA_EYE, "When to show");
    changed |= PluginUi::Check("Only while the game window is active", "av_fg", &s.onlyWhenForeground,
        "Hide the markers while another window is in front of the game.");
    changed |= PluginUi::Check("Hide while the POEFixer menu is open", "av_menu", &s.hideWhenMenuVisible,
        "Hide the markers while the POEFixer menu is shown.\n"
        "Leave it off to see changes live while you edit these settings.");
    PluginUi::Hint("Never drawn in towns and hideouts, or while the game is paused.");
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionMarker(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_marker");
    PluginUi::Heading(ICON_FA_BULLSEYE, "Marker on the monster");

    changed |= PluginUi::Check("Ring", "av_ring", &s.drawCircle, "Draw a ring around the monster.");
    changed |= PluginUi::Check("Name and cloud state", "av_label", &s.drawLabels,
        "Write \"AMANAMU VOID\" and whether it is in or out of its cloud above the monster.");
    changed |= PluginUi::Check("Show distance", "av_dist", &s.showDistance,
        "Add the distance from your character to the label and to the edge arrow.");

    ImGui::Spacing();
    const PluginUi::LabelColumn col{ "Ring size", "Label height" };
    ImGui::BeginDisabled(!s.drawCircle);
    changed |= PluginUi::SliderFloatRow(col, "Ring size", "###av_ring_size", &s.circleRadius,
        Limits::kRingMin, Limits::kRingMax, "%.0f px", "Radius of the ring at Scale 100%.");
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!s.drawLabels);
    changed |= PluginUi::SliderFloatRow(col, "Label height", "###av_label_h", &s.labelOffset,
        Limits::kLabelMin, Limits::kLabelMax, "%.0f px",
        "Space between the middle of the monster and the bottom of its label, at Scale 100%.");
    ImGui::EndDisabled();
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionArrow(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_arrow");
    PluginUi::Heading(ICON_FA_LOCATION_ARROW, "Edge arrow");
    changed |= PluginUi::Check("Point to monsters off screen", "av_arrows", &s.drawArrows,
        "Show an arrow at the screen edge in the direction of a tracked monster you cannot see.");
    ImGui::BeginDisabled(!s.drawArrows);
    changed |= PluginUi::Check("Also while the monster is on screen", "av_arrow_on", &s.arrowWhenOnScreen,
        "Keep the edge arrow even when the monster is visible.");
    ImGui::Spacing();
    const PluginUi::LabelColumn col{ "Distance from the edge" };
    changed |= PluginUi::SliderFloatRow(col, "Distance from the edge", "###av_margin", &s.arrowEdgeMargin,
        Limits::kMarginMin, Limits::kMarginMax, "%.0f px",
        "How far inside the screen edge the arrow sits, at Scale 100%.\n"
        "Raise it if the arrow hides behind the game's own UI.");
    ImGui::EndDisabled();
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionLook(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_look");
    PluginUi::Heading(ICON_FA_PALETTE, "Look");
    const PluginUi::LabelColumn col{ "In its cloud", "Left its cloud", "Scale", "Opacity" };
    changed |= ColorRow(col, "In its cloud", "###av_col_in", s.insideColor,
        "Colour while the monster stands in its void cloud.");
    changed |= ColorRow(col, "Left its cloud", "###av_col_out", s.outsideColor,
        "Colour once the monster has left its cloud.");
    col.Row("Scale");
    changed |= PercentSlider("###av_scale", s.scale, static_cast<int>(Limits::kScaleMin * 100.0f),
                             static_cast<int>(Limits::kScaleMax * 100.0f));
    Tip("Size of rings, arrows and text.\nIt also follows the POEFixer font size.");
    col.Row("Opacity");
    changed |= PercentSlider("###av_opacity", s.opacity, static_cast<int>(Limits::kOpacityMin * 100.0f), 100);
    Tip("Fade everything this plugin draws.");
    ImGui::Spacing();
    const char* resetLabel = ICON_FA_ROTATE_LEFT "  Default look###av_look_reset";
    PluginUi::AlignRight(PluginUi::ButtonWidth(resetLabel));
    if (PluginUi::SubtleButton(resetLabel)) {
        const Settings d;
        s.insideColor = d.insideColor;
        s.outsideColor = d.outsideColor;
        s.scale = d.scale;
        s.opacity = d.opacity;
        changed = true;
    }
    Tip("Restore the default colours, scale and opacity.");
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionDiagnostics(PanelContext& pc) {
    bool changed = false;
    if (PluginUi::BeginDiagnostics("Tracking counts, the debug window, detection logging and a reset of the markers.",
                                   "av_diag")) {
        const TrackerStats& st = pc.tracker.Stats();
        char line[192];
        std::snprintf(line, sizeof(line), "Tracked now: %d. Drawn this frame: %d (%.2f ms).",
                      static_cast<int>(pc.tracker.Monsters().size()), pc.overlay.drawn, pc.overlay.lastFrameUs / 1000.0);
        PluginUi::Hint(line);
        std::snprintf(line, sizeof(line), "Last pass: %d monsters, %d mod reads, %d buff reads, %.2f ms over %d frame(s).",
                      static_cast<int>(st.lastPassMonsters), st.modReads, st.buffReads, st.lastPassMs,
                      st.lastPassFrames);
        PluginUi::Hint(line);
        ImGui::Spacing();
        PluginUi::Check("Show the debug window", "av_debug", &pc.showDebugWindow,
            "Open a window listing every tracked monster with its modifiers and buffs.\n"
            "Not saved: it is closed again next time.");
        changed |= PluginUi::Check("Write new finds to the POEFixer log", "av_log", &pc.settings.logDetections,
            "Add one line to the POEFixer log each time a monster starts being tracked.");
        ImGui::Spacing();
        if (PluginUi::SubtleButton("Forget tracked monsters###av_forget")) pc.tracker.Reset();
        Tip("Drop every marker now.\nMonsters still around are found again within a moment.");
        PluginUi::EndDiagnostics();
    }
    return changed;
}

} // namespace

bool DrawSettingsPanel(PanelContext& pc) {
    if (!pc.hostCompatible) {
        ImGui::PushStyleColor(ImGuiCol_Text, PluginUi::Colors().Warning);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted("This POEFixer version is too old for Amanamu Void Alert: nothing is drawn. "
                               "Update POEFixer to use the plugin.");
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }
    bool changed = false;
    changed |= DrawSectionDetection(pc.settings);
    ImGui::Spacing();
    changed |= DrawSectionVisibility(pc.settings);
    ImGui::Spacing();
    changed |= DrawSectionMarker(pc.settings);
    ImGui::Spacing();
    changed |= DrawSectionArrow(pc.settings);
    ImGui::Spacing();
    changed |= DrawSectionLook(pc.settings);
    ImGui::Spacing();
    changed |= DrawSectionDiagnostics(pc);
    ImGui::Spacing();
    PluginUi::Hint("Originally created by 1k4ru5g3. Maintained by POEFixer.");
    ImGui::Spacing();
    return changed;
}

} // namespace AmanamuVoid
