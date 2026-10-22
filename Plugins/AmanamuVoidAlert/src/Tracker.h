// Finds Amanamu (Lightless Well) monsters and follows them.
//
// Detection runs as passes over the monster list (filtered before the SDK copies
// any strings), spread over frames within a time budget. A monster's mods never
// change, so they are read once per monster and remembered for the area. Only
// the few tracked monsters are refreshed every frame (position, life) and their
// buffs every 100 ms (inside or outside their cloud).
#pragma once

#include "../../../POEFixer/plugin_sdk/PluginSDK.h"
#include "EdgeMath.h"
#include "Settings.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace AmanamuVoid {

struct Tracked {
    uint32_t  id = 0;
    uintptr_t address = 0;
    uintptr_t buffsAddress = 0;
    std::string path;

    bool byMod = false;
    bool byBuff = false;
    bool insideCloud = false;

    float x = 0.0f, y = 0.0f, z = 0.0f;
    float height = 80.0f;            // model height (world units)
    float distance = 0.0f;

    double lastInRange = 0.0;        // seconds: last pass that found it within range
    double lastLive = 0.0;           // seconds: last frame the entity still existed
    double nextBuffRefresh = 0.0;

    bool hasDirection = false;
    Vec2 lastDirection{ 0.0f, -1.0f };

    std::vector<std::string> debugMods;
    std::vector<std::string> debugBuffs;
};

struct TrackerStats {
    size_t knownMonsters = 0;
    size_t lastPassMonsters = 0;
    double lastPassMs = 0.0;
    int    lastPassFrames = 0;
    int    modReads = 0;
    int    buffReads = 0;
};

class Tracker {
public:
    using Clock = std::chrono::steady_clock;

    void Reset();
    void Tick(const PluginSDK::Context& ctx, const Settings& s, double now);

    const std::vector<Tracked>& Monsters() const { return m_Tracked; }
    // Town, hideout or the game paused at the last pass: draw nothing.
    bool Suppressed() const { return m_Suppressed; }
    const PluginSDK::Entity& Player() const { return m_Player; }
    bool HasPlayer() const { return m_HasPlayer; }
    const std::string& AreaName() const { return m_AreaName; }
    const TrackerStats& Stats() const { return m_Stats; }
    std::vector<Tracked>& MutableMonsters() { return m_Tracked; }   // overlay caches arrow directions

private:
    struct Known {
        uintptr_t address = 0;
        uint32_t  pass = 0;
        bool      modsChecked = false;
        bool      hasMod = false;
        double    nextBuffCheck = 0.0;
        std::vector<std::string> debugMods;
    };
    struct BuffResult {
        bool scanned = false;
        bool lightless = false;
        bool inside = false;
        std::vector<std::string> relevant;
    };

    void BeginPass(const PluginSDK::Context& ctx, const Settings& s, double now);
    void Process(const PluginSDK::Context& ctx, const Settings& s, const PluginSDK::Entity& e, double now);
    void RefreshTracked(const PluginSDK::Context& ctx, const Settings& s, double now);
    BuffResult ReadBuffs(const PluginSDK::Context& ctx, uintptr_t buffsAddr);
    Tracked* Find(uint32_t id);

    std::unordered_map<uint32_t, Known> m_Known;
    std::vector<Tracked> m_Tracked;
    std::vector<PluginSDK::Entity> m_Pending;
    size_t m_Cursor = 0;
    bool   m_Active = false;
    uint32_t m_Pass = 0;
    double m_NextScan = 0.0;
    uint64_t m_AreaCounter = 0;
    bool   m_HaveArea = false;
    bool   m_Suppressed = false;
    std::string m_AreaName;

    PluginSDK::Entity m_Player;
    bool m_HasPlayer = false;
    float m_PassPlayerX = 0.0f, m_PassPlayerY = 0.0f;

    TrackerStats m_Stats;
    double m_PassMs = 0.0;
    int    m_PassFrames = 0;
    int    m_PassModReads = 0;
    int    m_PassBuffReads = 0;
};

} // namespace AmanamuVoid
