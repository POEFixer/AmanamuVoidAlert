// Pure 2D helpers for the off-screen arrow (no SDK, no ImGui), shared with the
// standalone test in tests/.
#pragma once

#include <algorithm>
#include <cmath>

namespace AmanamuVoid {

struct Vec2 {
    float x = 0.0f, y = 0.0f;
};

inline float Length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// Unit vector, or straight up when the input is (almost) zero or not finite.
inline Vec2 Normalize(Vec2 v) {
    const float len = Length(v);
    if (!(len > 0.001f) || !std::isfinite(len)) return Vec2{ 0.0f, -1.0f };
    return Vec2{ v.x / len, v.y / len };
}

// Where a ray from `origin` along `dir` leaves the screen rectangle shrunk by
// `margin` on every side. The origin is first pulled inside that rectangle, and
// the margin never exceeds 45 % of the smaller screen side, so the result is
// always on screen.
inline Vec2 ClampToScreenEdge(Vec2 origin, Vec2 dir, float width, float height, float margin) {
    margin = std::clamp(margin, 0.0f, std::min(width, height) * 0.45f);
    const float minX = margin, maxX = width - margin;
    const float minY = margin, maxY = height - margin;
    origin.x = std::clamp(origin.x, minX, maxX);
    origin.y = std::clamp(origin.y, minY, maxY);
    dir = Normalize(dir);

    float t = 1.0e9f;
    if (std::fabs(dir.x) > 1.0e-4f) t = std::min(t, ((dir.x > 0.0f ? maxX : minX) - origin.x) / dir.x);
    if (std::fabs(dir.y) > 1.0e-4f) t = std::min(t, ((dir.y > 0.0f ? maxY : minY) - origin.y) / dir.y);
    if (!(t >= 0.0f) || !std::isfinite(t)) t = 0.0f;
    return Vec2{ std::clamp(origin.x + dir.x * t, minX, maxX), std::clamp(origin.y + dir.y * t, minY, maxY) };
}

// Case-insensitive ASCII search; `lowerNeedle` must already be lower case.
inline bool ContainsNoCase(const char* hay, size_t hayLen, const char* lowerNeedle, size_t needleLen) {
    if (needleLen == 0 || hayLen < needleLen) return false;
    for (size_t i = 0; i + needleLen <= hayLen; ++i) {
        size_t j = 0;
        for (; j < needleLen; ++j) {
            char c = hay[i + j];
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            if (c != lowerNeedle[j]) break;
        }
        if (j == needleLen) return true;
    }
    return false;
}

} // namespace AmanamuVoid
