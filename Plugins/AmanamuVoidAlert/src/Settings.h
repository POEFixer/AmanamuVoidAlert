// Amanamu Void Alert settings: defaults, ranges, JSON persistence and the
// one-time migration of the upstream settings.txt. No SDK or ImGui dependency.
#pragma once

#include <filesystem>
#include <string>

namespace AmanamuVoid {

struct Color4 {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

namespace Limits {
constexpr float kDistanceMin = 500.0f,   kDistanceMax = 8000.0f;
constexpr float kKeepMin = 1.0f,         kKeepMax = 30.0f;
constexpr float kVanishMin = 0.2f,       kVanishMax = 5.0f;
constexpr float kRingMin = 12.0f,        kRingMax = 80.0f;
constexpr float kLabelMin = 20.0f,       kLabelMax = 140.0f;
constexpr float kMarginMin = 30.0f,      kMarginMax = 160.0f;
constexpr float kScaleMin = 0.6f,        kScaleMax = 2.0f;
constexpr float kOpacityMin = 0.2f,      kOpacityMax = 1.0f;
}

struct Settings {
    // Detection
    bool  onlyRareOrUnique = true;
    bool  detectByBuff = true;            // also track monsters that carry a Lightless Well buff
    float maxDistance = 3500.0f;          // game distance units
    float keepOutOfRangeSeconds = 15.0f;  // upstream "Forget after seconds"
    float forgetVanishedSeconds = 1.25f;  // upstream "Forget missing live entity after seconds"

    // When to show
    bool onlyWhenForeground = true;
    bool hideWhenMenuVisible = false;

    // Marker on the monster
    bool  drawCircle = true;
    float circleRadius = 34.0f;           // pixels at scale 1
    bool  drawLabels = true;
    float labelOffset = 70.0f;            // pixels from the marker centre to the label bottom
    bool  showDistance = true;

    // Edge arrow
    bool  drawArrows = true;
    bool  arrowWhenOnScreen = false;
    float arrowEdgeMargin = 95.0f;        // pixels from the screen edge

    // Look
    Color4 insideColor{ 0.71f, 0.31f, 1.00f, 1.0f };   // in its cloud
    Color4 outsideColor{ 0.31f, 1.00f, 0.47f, 1.0f };  // left its cloud
    float  scale = 1.0f;
    float  opacity = 1.0f;

    // Diagnostics
    bool logDetections = false;

    void Clamp();
};

bool ParseJson(const std::string& text, Settings& out);
bool ParseLegacy(const std::string& text, Settings& out);
std::string ToJson(const Settings& s);

// Unreadable: a settings file exists but could not be read (held open by another
// program, access denied, oversized). `out` holds the defaults; the caller must
// not write them over the file (see KeepUnreadableCopy).
enum class LoadSource { Json, Legacy, Defaults, CorruptJson, Unreadable };

LoadSource Load(const std::filesystem::path& pluginDir, Settings& out);
bool Save(const std::filesystem::path& pluginDir, const std::string& json);
// Best effort: copies an unreadable AmanamuVoidAlert.json to
// AmanamuVoidAlert.json.unreadable before the first save replaces it.
void KeepUnreadableCopy(const std::filesystem::path& pluginDir);

} // namespace AmanamuVoid
