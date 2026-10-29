#include "SettingsUi.h"

#include "Text.h"

#include "../../../POEFixer/plugin_sdk/PluginUi.h"
#include "../../../POEFixer/imgui/IconsFontAwesome6.h"

#include <cmath>
#include <string>

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
    const char* sample = I18n::Tr("marker_title");
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
    PluginUi::Heading(ICON_FA_CROSSHAIRS, I18n::Tr("detection_heading"), I18n::Tr("detection_help"));

    changed |= PluginUi::Check(I18n::Tr("detection_only_rare"), "av_rare", &s.onlyRareOrUnique,
        I18n::Tr("detection_only_rare_tip"));
    changed |= PluginUi::Check(I18n::Tr("detection_by_buff"), "av_bybuff", &s.detectByBuff,
        I18n::Tr("detection_by_buff_tip"));

    ImGui::Spacing();
    const char* range = I18n::Tr("detection_range");
    const char* keep = I18n::Tr("detection_keep");
    const char* forget = I18n::Tr("detection_forget");
    const PluginUi::LabelColumn col{ range, keep, forget };
    changed |= PluginUi::SliderFloatRow(col, range, "###av_range", &s.maxDistance,
        Limits::kDistanceMin, Limits::kDistanceMax, "%.0f", I18n::Tr("detection_range_tip"));
    changed |= PluginUi::SliderFloatRow(col, keep, "###av_keep", &s.keepOutOfRangeSeconds,
        Limits::kKeepMin, Limits::kKeepMax, UnitFormat(I18n::Tr("unit_seconds"), "%.0f").c_str(),
        I18n::Tr("detection_keep_tip"));
    changed |= PluginUi::SliderFloatRow(col, forget, "###av_vanish", &s.forgetVanishedSeconds,
        Limits::kVanishMin, Limits::kVanishMax, UnitFormat(I18n::Tr("unit_seconds"), "%.2f").c_str(),
        I18n::Tr("detection_forget_tip"));
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionVisibility(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_when");
    PluginUi::Heading(ICON_FA_EYE, I18n::Tr("visibility_heading"));
    changed |= PluginUi::Check(I18n::Tr("visibility_foreground"), "av_fg", &s.onlyWhenForeground,
        I18n::Tr("visibility_foreground_tip"));
    changed |= PluginUi::Check(I18n::Tr("visibility_hide_menu"), "av_menu", &s.hideWhenMenuVisible,
        I18n::Tr("visibility_hide_menu_tip"));
    PluginUi::Hint(I18n::Tr("visibility_hint"));
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionMarker(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_marker");
    PluginUi::Heading(ICON_FA_BULLSEYE, I18n::Tr("marker_heading"));

    changed |= PluginUi::Check(I18n::Tr("marker_ring"), "av_ring", &s.drawCircle, I18n::Tr("marker_ring_tip"));
    const std::string labelTip = I18n::TrF("marker_label_tip", { { "label", I18n::Tr("marker_title") } });
    changed |= PluginUi::Check(I18n::Tr("marker_label"), "av_label", &s.drawLabels, labelTip.c_str());
    changed |= PluginUi::Check(I18n::Tr("marker_distance"), "av_dist", &s.showDistance,
        I18n::Tr("marker_distance_tip"));

    ImGui::Spacing();
    const char* ringSize = I18n::Tr("marker_ring_size");
    const char* labelHeight = I18n::Tr("marker_label_height");
    const PluginUi::LabelColumn col{ ringSize, labelHeight };
    ImGui::BeginDisabled(!s.drawCircle);
    changed |= PluginUi::SliderFloatRow(col, ringSize, "###av_ring_size", &s.circleRadius,
        Limits::kRingMin, Limits::kRingMax, UnitFormat(I18n::Tr("unit_pixels"), "%.0f").c_str(),
        I18n::Tr("marker_ring_size_tip"));
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!s.drawLabels);
    changed |= PluginUi::SliderFloatRow(col, labelHeight, "###av_label_h", &s.labelOffset,
        Limits::kLabelMin, Limits::kLabelMax, UnitFormat(I18n::Tr("unit_pixels"), "%.0f").c_str(),
        I18n::Tr("marker_label_height_tip"));
    ImGui::EndDisabled();
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionArrow(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_arrow");
    PluginUi::Heading(ICON_FA_LOCATION_ARROW, I18n::Tr("arrow_heading"));
    changed |= PluginUi::Check(I18n::Tr("arrow_off_screen"), "av_arrows", &s.drawArrows,
        I18n::Tr("arrow_off_screen_tip"));
    ImGui::BeginDisabled(!s.drawArrows);
    changed |= PluginUi::Check(I18n::Tr("arrow_on_screen"), "av_arrow_on", &s.arrowWhenOnScreen,
        I18n::Tr("arrow_on_screen_tip"));
    ImGui::Spacing();
    const char* margin = I18n::Tr("arrow_margin");
    const PluginUi::LabelColumn col{ margin };
    changed |= PluginUi::SliderFloatRow(col, margin, "###av_margin", &s.arrowEdgeMargin,
        Limits::kMarginMin, Limits::kMarginMax, UnitFormat(I18n::Tr("unit_pixels"), "%.0f").c_str(),
        I18n::Tr("arrow_margin_tip"));
    ImGui::EndDisabled();
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionLook(Settings& s) {
    bool changed = false;
    PluginUi::BeginCard("###av_card_look");
    PluginUi::Heading(ICON_FA_PALETTE, I18n::Tr("look_heading"));
    const char* inside = I18n::Tr("look_inside");
    const char* outside = I18n::Tr("look_outside");
    const char* scale = I18n::Tr("look_scale");
    const char* opacity = I18n::Tr("look_opacity");
    const PluginUi::LabelColumn col{ inside, outside, scale, opacity };
    changed |= ColorRow(col, inside, "###av_col_in", s.insideColor, I18n::Tr("look_inside_tip"));
    changed |= ColorRow(col, outside, "###av_col_out", s.outsideColor, I18n::Tr("look_outside_tip"));
    col.Row(scale);
    changed |= PercentSlider("###av_scale", s.scale, static_cast<int>(Limits::kScaleMin * 100.0f),
                             static_cast<int>(Limits::kScaleMax * 100.0f));
    Tip(I18n::Tr("look_scale_tip"));
    col.Row(opacity);
    changed |= PercentSlider("###av_opacity", s.opacity, static_cast<int>(Limits::kOpacityMin * 100.0f), 100);
    Tip(I18n::Tr("look_opacity_tip"));
    ImGui::Spacing();
    const std::string resetLabel = std::string(ICON_FA_ROTATE_LEFT "  ") + I18n::Tr("look_reset") + "###av_look_reset";
    PluginUi::AlignRight(PluginUi::ButtonWidth(resetLabel.c_str()));
    if (PluginUi::SubtleButton(resetLabel.c_str())) {
        const Settings d;
        s.insideColor = d.insideColor;
        s.outsideColor = d.outsideColor;
        s.scale = d.scale;
        s.opacity = d.opacity;
        changed = true;
    }
    Tip(I18n::Tr("look_reset_tip"));
    PluginUi::EndCard();
    return changed;
}

bool DrawSectionDiagnostics(PanelContext& pc) {
    bool changed = false;
    if (PluginUi::BeginDiagnostics(I18n::Tr("diag_tip"), "av_diag")) {
        const TrackerStats& st = pc.tracker.Stats();
        PluginUi::Hint(I18n::TrF("diag_tracked_now", {
            { "tracked", Num("%d", static_cast<int>(pc.tracker.Monsters().size())) },
            { "drawn", Num("%d", pc.overlay.drawn) },
            { "ms", Num("%.2f", pc.overlay.lastFrameUs / 1000.0) } }).c_str());
        PluginUi::Hint(LastPassText(st).c_str());
        ImGui::Spacing();
        PluginUi::Check(I18n::Tr("diag_debug_window"), "av_debug", &pc.showDebugWindow,
            I18n::Tr("diag_debug_window_tip"));
        changed |= PluginUi::Check(I18n::Tr("diag_log"), "av_log", &pc.settings.logDetections,
            I18n::Tr("diag_log_tip"));
        ImGui::Spacing();
        if (PluginUi::SubtleButton(I18n::TrId("diag_forget"))) pc.tracker.Reset();
        Tip(I18n::Tr("diag_forget_tip"));
        PluginUi::EndDiagnostics();
    }
    return changed;
}

} // namespace

bool DrawSettingsPanel(PanelContext& pc) {
    if (!pc.hostCompatible) {
        ImGui::PushStyleColor(ImGuiCol_Text, PluginUi::Colors().Warning);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(I18n::Tr("host_too_old"));
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
