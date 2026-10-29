#include "Overlay.h"

#include "Text.h"

#include "../../../POEFixer/plugin_sdk/PluginUi.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

namespace AmanamuVoid {

namespace {

constexpr float kReferenceFont = 16.0f;   // sizes in the settings are pixels at this font size

ImU32 ToU32(const Color4& c, float alphaScale) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, std::clamp(c.a * alphaScale, 0.0f, 1.0f)));
}

ImVec2 ToIm(Vec2 v) { return ImVec2(v.x, v.y); }

bool Project(const PluginSDK::Context& ctx, float x, float y, float z, Vec2& out) {
    float sx = 0.0f, sy = 0.0f;
    if (!ctx.Render.WorldToScreen(x, y, z, sx, sy) || !std::isfinite(sx) || !std::isfinite(sy)) return false;
    out = Vec2{ sx, sy };
    return true;
}

// Screen direction from the player to the monster. The host only projects points
// that are on screen, so for an off-screen monster a point a short way from the
// player towards it is projected instead (shorter steps until one is on screen).
Vec2 DirectionTo(const PluginSDK::Context& ctx, const Tracker& tracker, Tracked& t, Vec2 origin, bool haveOrigin,
                 bool bodyVisible, Vec2 body) {
    if (bodyVisible && haveOrigin) {
        const Vec2 d{ body.x - origin.x, body.y - origin.y };
        if (Length(d) > 2.0f) {
            t.lastDirection = Normalize(d);
            t.hasDirection = true;
            return t.lastDirection;
        }
    }
    if (tracker.HasPlayer() && haveOrigin) {
        const PluginSDK::Entity& p = tracker.Player();
        const Vec2 world{ t.x - p.WorldX, t.y - p.WorldY };
        const float dist = Length(world);
        if (dist > 1.0f) {
            const Vec2 n{ world.x / dist, world.y / dist };
            for (float step : { 600.0f, 300.0f, 150.0f, 75.0f, 40.0f }) {
                const float d = std::min(step, dist);
                Vec2 proxy;
                if (!Project(ctx, p.WorldX + n.x * d, p.WorldY + n.y * d, p.WorldZ, proxy)) continue;
                const Vec2 sd{ proxy.x - origin.x, proxy.y - origin.y };
                if (Length(sd) > 2.0f) {
                    t.lastDirection = Normalize(sd);
                    t.hasDirection = true;
                    return t.lastDirection;
                }
            }
        }
    }
    return t.hasDirection ? t.lastDirection : Vec2{ 0.0f, -1.0f };
}

void DrawArrow(ImDrawList* dl, Vec2 pos, Vec2 dir, float size, ImU32 color, ImU32 edge) {
    const Vec2 perp{ -dir.y, dir.x };
    const ImVec2 tip(pos.x + dir.x * size, pos.y + dir.y * size);
    const Vec2 back{ pos.x - dir.x * size * 0.65f, pos.y - dir.y * size * 0.65f };
    const ImVec2 l(back.x + perp.x * size * 0.6f, back.y + perp.y * size * 0.6f);
    const ImVec2 r(back.x - perp.x * size * 0.6f, back.y - perp.y * size * 0.6f);
    dl->AddTriangleFilled(tip, l, r, color);
    // Centred edge (ImGui 1.93 strokes a closed shape inside by default, which would
    // eat into a small arrow).
    dl->AddTriangle(tip, l, r, edge, std::max(1.0f, size * 0.09f), ImDrawFlags_StrokeCenter);
}

// Text centred on `center`, kept fully on screen.
void CenteredText(ImDrawList* dl, float fontSize, Vec2 center, float w, float h, ImU32 color, const char* text) {
    const ImVec2 size = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    float x = center.x - size.x * 0.5f;
    float y = center.y - size.y * 0.5f;
    x = std::clamp(x, 4.0f, std::max(4.0f, w - size.x - 4.0f));
    y = std::clamp(y, 4.0f, std::max(4.0f, h - size.y - 4.0f));
    PluginUi::TextOutlined(dl, nullptr, fontSize, ImVec2(x, y), color, text);
}

} // namespace

void DrawOverlay(const PluginSDK::Context& ctx, const Settings& s, Tracker& tracker, OverlayStats& stats) {
    const auto t0 = std::chrono::steady_clock::now();
    stats.drawn = 0;
    if (tracker.Suppressed() || tracker.Monsters().empty()) {
        stats.lastFrameUs = 0.0;
        return;
    }
    const PluginSDK::ScreenSize screen = ctx.Game.GetScreenSize();
    const float w = screen.Width, h = screen.Height;
    if (!(w > 0.0f) || !(h > 0.0f)) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    const float k = std::clamp(s.scale, Limits::kScaleMin, Limits::kScaleMax) * ImGui::GetFontSize() / kReferenceFont;
    const float fontSize = ImGui::GetFontSize() * std::clamp(s.scale, Limits::kScaleMin, Limits::kScaleMax);
    const float opacity = std::clamp(s.opacity, Limits::kOpacityMin, Limits::kOpacityMax);
    const ImU32 edge = IM_COL32(0, 0, 0, static_cast<int>(200.0f * opacity));

    Vec2 origin{ w * 0.5f, h * 0.5f };
    bool haveOrigin = true;
    if (tracker.HasPlayer()) {
        const PluginSDK::Entity& p = tracker.Player();
        Vec2 ps;
        if (Project(ctx, p.WorldX, p.WorldY, p.WorldZ, ps)) origin = ps;
    }

    for (Tracked& t : tracker.MutableMonsters()) {
        const ImU32 color = ToU32(t.insideCloud ? s.insideColor : s.outsideColor, opacity);
        Vec2 body;
        const bool visible = Project(ctx, t.x, t.y, t.z - t.height * 0.5f, body);
        const std::string dist = s.showDistance ? "  " + Num("%.0f", t.distance) : std::string();

        if (visible) {
            ++stats.drawn;
            const ImVec2 c = PluginUi::SnapPx(ToIm(body));
            if (s.drawCircle) {
                const float r = s.circleRadius * k;
                const float th = std::max(1.5f, 2.5f * k);
                // Both strokes centred on the radius so the dark edge shows on both sides
                // of the coloured ring (ImGui 1.93 strokes a circle inside by default).
                dl->AddCircle(c, r, edge, 0, th + 2.0f, ImDrawFlags_StrokeCenter);
                dl->AddCircle(c, r, color, 0, th, ImDrawFlags_StrokeCenter);
            }
            if (s.drawLabels) {
                const std::string line2 =
                    std::string(t.insideCloud ? I18n::Tr("overlay_in_cloud") : I18n::Tr("overlay_out_of_cloud")) + dist;
                const char* line1 = I18n::Tr("marker_title");
                const ImVec2 s1 = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line1);
                const ImVec2 s2 = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line2.c_str());
                const float bottom = c.y - s.labelOffset * k;
                CenteredText(dl, fontSize, Vec2{ c.x, bottom - s2.y - s1.y * 0.5f }, w, h, color, line1);
                CenteredText(dl, fontSize, Vec2{ c.x, bottom - s2.y * 0.5f }, w, h, color, line2.c_str());
            }
        }

        if (s.drawArrows && (!visible || s.arrowWhenOnScreen)) {
            const Vec2 dir = DirectionTo(ctx, tracker, t, origin, haveOrigin, visible, body);
            const float size = 18.0f * k;
            const Vec2 pos = ClampToScreenEdge(origin, dir, w, h, std::max(s.arrowEdgeMargin * k, size * 1.5f));
            DrawArrow(dl, Vec2{ std::floor(pos.x + 0.5f), std::floor(pos.y + 0.5f) }, dir, size, color, edge);

            const std::string text = std::string(t.insideCloud ? I18n::Tr("overlay_arrow_in_cloud")
                                                               : I18n::Tr("overlay_arrow_out_of_cloud")) + dist;
            const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str());
            const float back = size * 1.1f + std::max(ts.x, ts.y) * 0.55f;
            CenteredText(dl, fontSize, Vec2{ pos.x - dir.x * back, pos.y - dir.y * back }, w, h, color, text.c_str());
            if (!visible) ++stats.drawn;
        }
    }
    stats.lastFrameUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
}

void DrawDebugWindow(const Tracker& tracker, bool* open, double now) {
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2(fs * 52.0f, fs * 26.0f), ImGuiCond_FirstUseEver);
    const std::string title = std::string(I18n::Tr("debug_title")) + "###amanamu_void_debug";
    if (!ImGui::Begin(title.c_str(), open)) {
        ImGui::End();
        return;
    }
    const TrackerStats& st = tracker.Stats();
    ImGui::TextUnformatted(I18n::TrF("debug_area", { { "area", tracker.AreaName().empty() ? "-" : tracker.AreaName() } }).c_str());
    ImGui::TextUnformatted(I18n::TrF("debug_tracked", { { "count", Num("%d", static_cast<int>(tracker.Monsters().size())) } }).c_str());
    ImGui::SameLine(0.0f, ImGui::GetFontSize());
    ImGui::TextUnformatted(I18n::TrF("debug_remembered", { { "count", Num("%d", static_cast<int>(st.knownMonsters)) } }).c_str());
    ImGui::TextUnformatted(LastPassText(st).c_str());
    ImGui::Separator();

    // Not resizable: the fixed columns (width 0) fit their translated header and cells every frame.
    const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX;
    if (ImGui::BeginTable("###amanamu_tracked", 7, flags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_id"), ImGuiTableColumnFlags_WidthFixed, 0.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_cloud"), ImGuiTableColumnFlags_WidthFixed, 0.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_found_by"), ImGuiTableColumnFlags_WidthFixed, 0.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_distance"), ImGuiTableColumnFlags_WidthFixed, 0.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_last_seen"), ImGuiTableColumnFlags_WidthFixed, 0.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_path"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(I18n::TrId("debug_col_mods"), ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableHeadersRow();
        for (const Tracked& t : tracker.Monsters()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%u", t.id);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(t.insideCloud ? I18n::Tr("debug_inside") : I18n::Tr("debug_outside"));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(t.byMod && t.byBuff ? I18n::Tr("debug_found_both")
                                   : (t.byMod ? I18n::Tr("debug_found_mod") : I18n::Tr("debug_found_buff")));
            ImGui::TableNextColumn();
            ImGui::Text("%.0f", t.distance);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(
                I18n::TrF("unit_seconds", { { "value", Num("%.1f", std::max(0.0, now - t.lastLive)) } }).c_str());
            ImGui::TableNextColumn();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(t.path.c_str());
            ImGui::TableNextColumn();
            for (const std::string& m : t.debugMods) ImGui::TextUnformatted(m.c_str());
            for (const std::string& b : t.debugBuffs) ImGui::TextDisabled("%s", b.c_str());
            if (t.debugMods.empty() && t.debugBuffs.empty()) ImGui::TextDisabled("-");
            ImGui::PopTextWrapPos();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

std::string LastPassText(const TrackerStats& st) {
    return I18n::TrF("diag_last_pass", {
        { "monsters", Num("%d", static_cast<int>(st.lastPassMonsters)) },
        { "mod_reads", Num("%d", st.modReads) },
        { "buff_reads", Num("%d", st.buffReads) },
        { "ms", Num("%.2f", st.lastPassMs) },
        { "frames", Num("%d", st.lastPassFrames) } });
}

} // namespace AmanamuVoid
