#include "Settings.h"

#include "AtomicFile.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <string_view>

namespace AmanamuVoid {

using nlohmann::json;

namespace {

constexpr const char* kJsonName = "AmanamuVoidAlert.json";
constexpr const char* kLegacyName = "settings.txt";

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

float ClampF(float v, float lo, float hi, float fallback) {
    if (!(v == v)) return fallback;   // NaN
    return std::clamp(v, lo, hi);
}

void ReadBool(const json& j, const char* key, bool& v) {
    auto it = j.find(key);
    if (it != j.end() && it->is_boolean()) v = it->get<bool>();
}
void ReadFloat(const json& j, const char* key, float& v) {
    auto it = j.find(key);
    if (it != j.end() && it->is_number()) v = static_cast<float>(it->get<double>());
}
void ReadColor(const json& j, const char* key, Color4& c) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_array() || it->size() != 4) return;
    float v[4];
    for (size_t i = 0; i < 4; ++i) {
        if (!(*it)[i].is_number()) return;
        v[i] = Clamp01(static_cast<float>((*it)[i].get<double>()));
    }
    c = Color4{ v[0], v[1], v[2], v[3] };
}

bool ParseFloat(std::string_view s, float& out) {
    while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r')) s.remove_suffix(1);
    float v = 0.0f;
    auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec != std::errc()) return false;
    out = v;
    return true;
}

} // namespace

void Settings::Clamp() {
    const Settings d;
    maxDistance = ClampF(maxDistance, Limits::kDistanceMin, Limits::kDistanceMax, d.maxDistance);
    keepOutOfRangeSeconds = ClampF(keepOutOfRangeSeconds, Limits::kKeepMin, Limits::kKeepMax, d.keepOutOfRangeSeconds);
    forgetVanishedSeconds = ClampF(forgetVanishedSeconds, Limits::kVanishMin, Limits::kVanishMax, d.forgetVanishedSeconds);
    circleRadius = ClampF(circleRadius, Limits::kRingMin, Limits::kRingMax, d.circleRadius);
    labelOffset = ClampF(labelOffset, Limits::kLabelMin, Limits::kLabelMax, d.labelOffset);
    arrowEdgeMargin = ClampF(arrowEdgeMargin, Limits::kMarginMin, Limits::kMarginMax, d.arrowEdgeMargin);
    scale = ClampF(scale, Limits::kScaleMin, Limits::kScaleMax, d.scale);
    opacity = ClampF(opacity, Limits::kOpacityMin, Limits::kOpacityMax, d.opacity);
    for (Color4* c : { &insideColor, &outsideColor })
        *c = Color4{ Clamp01(c->r), Clamp01(c->g), Clamp01(c->b), Clamp01(c->a) };
}

bool ParseJson(const std::string& text, Settings& out) {
    json j = json::parse(text, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
    if (j.is_discarded() || !j.is_object()) return false;
    Settings s;
    ReadBool(j, "onlyRareOrUnique", s.onlyRareOrUnique);
    ReadBool(j, "detectByBuff", s.detectByBuff);
    ReadFloat(j, "maxDistance", s.maxDistance);
    ReadFloat(j, "keepOutOfRangeSeconds", s.keepOutOfRangeSeconds);
    ReadFloat(j, "forgetVanishedSeconds", s.forgetVanishedSeconds);
    ReadBool(j, "onlyWhenForeground", s.onlyWhenForeground);
    ReadBool(j, "hideWhenMenuVisible", s.hideWhenMenuVisible);
    ReadBool(j, "drawCircle", s.drawCircle);
    ReadFloat(j, "circleRadius", s.circleRadius);
    ReadBool(j, "drawLabels", s.drawLabels);
    ReadFloat(j, "labelOffset", s.labelOffset);
    ReadBool(j, "showDistance", s.showDistance);
    ReadBool(j, "drawArrows", s.drawArrows);
    ReadBool(j, "arrowWhenOnScreen", s.arrowWhenOnScreen);
    ReadFloat(j, "arrowEdgeMargin", s.arrowEdgeMargin);
    ReadColor(j, "insideColor", s.insideColor);
    ReadColor(j, "outsideColor", s.outsideColor);
    ReadFloat(j, "scale", s.scale);
    ReadFloat(j, "opacity", s.opacity);
    ReadBool(j, "logDetections", s.logDetections);
    s.Clamp();
    out = s;
    return true;
}

bool ParseLegacy(const std::string& text, Settings& out) {
    Settings s;
    bool any = false;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        std::string_view line(text.data() + pos, eol - pos);
        pos = eol + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        const size_t eq = line.find('=');
        if (eq == std::string_view::npos) continue;
        const std::string_view key = line.substr(0, eq);
        const std::string_view value = line.substr(eq + 1);
        const bool b = value == "1";
        any = true;
        // EnableOverlay is the host's Enabled checkbox now; ShowDebugWindow is
        // session-only; ProxyDistance is no longer needed by the arrow.
        if (key == "DrawOnScreenLabels") s.drawLabels = b;
        else if (key == "DrawOffscreenArrows") s.drawArrows = b;
        else if (key == "DrawEdgeArrowForOnScreenMonsters") s.arrowWhenOnScreen = b;
        else if (key == "DrawCircle") s.drawCircle = b;
        else if (key == "OnlyRareOrUnique") s.onlyRareOrUnique = b;
        else if (key == "LogNewDetections") s.logDetections = b;
        else if (key == "MaxDistance") ParseFloat(value, s.maxDistance);
        else if (key == "ForgetAfterSeconds") ParseFloat(value, s.keepOutOfRangeSeconds);
        else if (key == "MissingEntityForgetSeconds") ParseFloat(value, s.forgetVanishedSeconds);
        else if (key == "LabelYOffset") ParseFloat(value, s.labelOffset);
        else if (key == "CircleRadius") ParseFloat(value, s.circleRadius);
        else if (key == "ArrowEdgeMargin") ParseFloat(value, s.arrowEdgeMargin);
    }
    if (!any) return false;
    s.Clamp();
    out = s;
    return true;
}

std::string ToJson(const Settings& s) {
    json j;
    j["version"] = 1;
    j["onlyRareOrUnique"] = s.onlyRareOrUnique;
    j["detectByBuff"] = s.detectByBuff;
    j["maxDistance"] = s.maxDistance;
    j["keepOutOfRangeSeconds"] = s.keepOutOfRangeSeconds;
    j["forgetVanishedSeconds"] = s.forgetVanishedSeconds;
    j["onlyWhenForeground"] = s.onlyWhenForeground;
    j["hideWhenMenuVisible"] = s.hideWhenMenuVisible;
    j["drawCircle"] = s.drawCircle;
    j["circleRadius"] = s.circleRadius;
    j["drawLabels"] = s.drawLabels;
    j["labelOffset"] = s.labelOffset;
    j["showDistance"] = s.showDistance;
    j["drawArrows"] = s.drawArrows;
    j["arrowWhenOnScreen"] = s.arrowWhenOnScreen;
    j["arrowEdgeMargin"] = s.arrowEdgeMargin;
    j["insideColor"] = { s.insideColor.r, s.insideColor.g, s.insideColor.b, s.insideColor.a };
    j["outsideColor"] = { s.outsideColor.r, s.outsideColor.g, s.outsideColor.b, s.outsideColor.a };
    j["scale"] = s.scale;
    j["opacity"] = s.opacity;
    j["logDetections"] = s.logDetections;
    return j.dump(2, ' ', false, json::error_handler_t::replace);
}

LoadSource Load(const std::filesystem::path& pluginDir, Settings& out) {
    out = Settings{};
    if (pluginDir.empty()) return LoadSource::Defaults;
    const std::filesystem::path dir = pluginDir / L"config";
    std::string text;
    const std::filesystem::path jsonPath = dir / kJsonName;
    if (AtomicFile::ReadAll(jsonPath, text)) {
        if (ParseJson(text, out)) return LoadSource::Json;
        std::filesystem::path aside = jsonPath;
        aside += L".corrupt";
        ::MoveFileExW(jsonPath.c_str(), aside.c_str(), MOVEFILE_REPLACE_EXISTING);
        return LoadSource::CorruptJson;
    }
    // Present but not readable (locked, denied, oversized): not a first start.
    if (AtomicFile::Present(jsonPath)) return LoadSource::Unreadable;
    const std::filesystem::path legacyPath = dir / kLegacyName;
    if (AtomicFile::ReadAll(legacyPath, text)) {
        if (ParseLegacy(text, out)) return LoadSource::Legacy;
    } else if (AtomicFile::Present(legacyPath)) {
        return LoadSource::Unreadable;
    }
    return LoadSource::Defaults;
}

void KeepUnreadableCopy(const std::filesystem::path& pluginDir) {
    if (pluginDir.empty()) return;
    const std::filesystem::path jsonPath = pluginDir / L"config" / kJsonName;
    std::filesystem::path aside = jsonPath;
    aside += L".unreadable";
    ::CopyFileW(jsonPath.c_str(), aside.c_str(), FALSE);
}

bool Save(const std::filesystem::path& pluginDir, const std::string& jsonText) {
    if (pluginDir.empty()) return false;
    return AtomicFile::WriteAll(pluginDir / L"config" / kJsonName, jsonText);
}

} // namespace AmanamuVoid
