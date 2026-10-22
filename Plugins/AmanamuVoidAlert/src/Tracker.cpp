#include "Tracker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace AmanamuVoid {

namespace {

// Primary detection: the Lightless faction monster mod (Mods.dat Id, its monster
// metadata, or its Hash32). The 16-bit hash is not used: too many mods share it.
constexpr const char* kModId = "MonsterAbyssLightlessFaction1";
constexpr const char* kModMetadata = "Metadata/Monsters/MonsterMods/LeagueAbyss/LightlessWells";
constexpr uint32_t    kModHash32 = 0xBFDA2A36u;

// Cloud state: the monster is immune while it stands in its own cloud.
constexpr const char kBuffLightless[] = "abyss_lightless_well";
constexpr const char kBuffInside[] = "abyss_lightless_well_immune";

constexpr double kPassIntervalSeconds = 0.15;
constexpr double kBuffRecheckSeconds = 1.0;     // an untracked monster's buffs
constexpr double kTrackedBuffSeconds = 0.10;    // a tracked monster's buffs
constexpr auto   kFrameBudget = std::chrono::microseconds(1500);
constexpr size_t kMaxTracked = 24;
constexpr size_t kMaxDebugLines = 8;

bool Has(const std::string& hay, const char* lowerNeedle) {
    return ContainsNoCase(hay.data(), hay.size(), lowerNeedle, std::strlen(lowerNeedle));
}

bool IsRelevantText(const std::string& s) {
    return Has(s, "abyss") || Has(s, "lightless") || Has(s, "amanamu");
}

std::string Narrow(const std::wstring& w) {
    std::string out;
    out.reserve(w.size());
    for (wchar_t c : w) out.push_back(c < 0x80 ? static_cast<char>(c) : '?');
    return out;
}

float Distance2D(float ax, float ay, float bx, float by) {
    const float dx = ax - bx, dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}

bool IsDead(const PluginSDK::Entity& e) { return e.MaxHP > 0 && e.CurrentHP <= 0; }

} // namespace

void Tracker::Reset() {
    m_Known.clear();
    m_Tracked.clear();
    m_Pending.clear();
    m_Cursor = 0;
    m_Active = false;
    m_NextScan = 0.0;
    m_HaveArea = false;
    m_Suppressed = false;
    m_HasPlayer = false;
    m_Stats = TrackerStats{};
}

Tracked* Tracker::Find(uint32_t id) {
    for (Tracked& t : m_Tracked)
        if (t.id == id) return &t;
    return nullptr;
}

void Tracker::Tick(const PluginSDK::Context& ctx, const Settings& s, double now) {
    if (!m_Active && now >= m_NextScan) BeginPass(ctx, s, now);

    if (m_Active) {
        const auto start = Clock::now();
        int work = 0;
        while (m_Cursor < m_Pending.size()) {
            const int readsBefore = m_PassModReads + m_PassBuffReads;
            Process(ctx, s, m_Pending[m_Cursor++], now);
            work += (m_PassModReads + m_PassBuffReads != readsBefore) ? 8 : 1;
            if (work >= 32) {
                work = 0;
                if (Clock::now() - start >= kFrameBudget) break;
            }
        }
        m_PassMs += std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        ++m_PassFrames;
        if (m_Cursor >= m_Pending.size()) {
            m_Active = false;
            m_Stats.lastPassMonsters = m_Pending.size();
            m_Stats.lastPassMs = m_PassMs;
            m_Stats.lastPassFrames = m_PassFrames;
            m_Stats.modReads = m_PassModReads;
            m_Stats.buffReads = m_PassBuffReads;
            m_Pending.clear();
            for (auto it = m_Known.begin(); it != m_Known.end();) {
                if (it->second.pass != m_Pass) it = m_Known.erase(it);
                else ++it;
            }
            m_Stats.knownMonsters = m_Known.size();
        }
    }

    RefreshTracked(ctx, s, now);
}

void Tracker::BeginPass(const PluginSDK::Context& ctx, const Settings& s, double now) {
    (void)s;
    m_NextScan = now + kPassIntervalSeconds;
    // Towns and hideouts: a cheap flag read instead of an entity list.
    if (ctx.Game.IsTownOrHideout()) {
        m_Tracked.clear();
        m_Suppressed = true;
        return;
    }
    const auto start = Clock::now();
    PluginSDK::Snapshot snap = ctx.Game.GetSnapshot(PluginSDK::EntityType::Monster);
    const double snapMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

    if (!snap.IsAttached || snap.State != PluginSDK::GameState::InGame) {
        Reset();
        m_NextScan = now + kPassIntervalSeconds;
        return;
    }
    if (!m_HaveArea || snap.AreaChangeCounter != m_AreaCounter) {
        m_Known.clear();
        m_Tracked.clear();
        m_AreaCounter = snap.AreaChangeCounter;
        m_HaveArea = true;
    }
    m_AreaName = snap.CurrentAreaName;
    m_Suppressed = snap.IsTown || snap.IsHideout || snap.IsPaused;
    if (snap.IsTown || snap.IsHideout) {
        m_Tracked.clear();
        return;
    }
    if (snap.IsPaused) return;

    m_PassPlayerX = snap.Player.WorldX;
    m_PassPlayerY = snap.Player.WorldY;
    m_Pending = std::move(snap.Entities);
    m_Cursor = 0;
    m_Active = true;
    ++m_Pass;
    m_PassMs = snapMs;
    m_PassFrames = 0;
    m_PassModReads = 0;
    m_PassBuffReads = 0;
}

Tracker::BuffResult Tracker::ReadBuffs(const PluginSDK::Context& ctx, uintptr_t buffsAddr) {
    BuffResult r;
    if (!buffsAddr) return r;
    ++m_PassBuffReads;
    const std::vector<PluginSDK::Buff> buffs = ctx.Components.EnumerateBuffs(buffsAddr);
    r.scanned = true;
    for (const PluginSDK::Buff& b : buffs) {
        if (Has(b.Name, kBuffLightless)) {
            r.lightless = true;
            if (Has(b.Name, kBuffInside)) r.inside = true;
        }
        if (r.relevant.size() < kMaxDebugLines && (IsRelevantText(b.Name) || Has(b.Name, "well")) &&
            std::find(r.relevant.begin(), r.relevant.end(), b.Name) == r.relevant.end())
            r.relevant.push_back(b.Name);
    }
    return r;
}

void Tracker::Process(const PluginSDK::Context& ctx, const Settings& s, const PluginSDK::Entity& e, double now) {
    if (!e.IsValid || e.Address == 0) return;
    if (e.EntityState == PluginSDK::EntityState::MonsterFriendly || IsDead(e)) return;
    if (s.onlyRareOrUnique && e.Rarity < 2) return;
    if (!e.Components.HasBuffs() && !e.Components.HasOMP()) return;

    auto [it, inserted] = m_Known.try_emplace(e.Id);
    Known& known = it->second;
    if (!inserted && known.address != e.Address) known = Known{};   // the id now names another monster
    known.address = e.Address;
    known.pass = m_Pass;

    const float distance = Distance2D(e.WorldX, e.WorldY, m_PassPlayerX, m_PassPlayerY);
    if (distance > s.maxDistance) return;

    if (!known.modsChecked && e.Components.HasOMP()) {
        ++m_PassModReads;
        known.modsChecked = true;
        for (const PluginSDK::MonsterMod& mod : ctx.Components.EnumerateMonsterMods(e.Components.OMP)) {
            const bool match = mod.Id == kModId || mod.Metadata == kModMetadata || mod.Hash32 == kModHash32;
            known.hasMod |= match;
            if ((match || IsRelevantText(mod.Id) || IsRelevantText(mod.Metadata)) &&
                known.debugMods.size() < kMaxDebugLines) {
                char line[512];
                std::snprintf(line, sizeof(line), "%s | %s | h32=0x%08X", mod.Id.c_str(), mod.Metadata.c_str(),
                              static_cast<unsigned int>(mod.Hash32));
                known.debugMods.emplace_back(line);
            }
        }
    }

    Tracked* t = Find(e.Id);
    if (t && t->address != e.Address) {
        // Same id, other monster: drop the old entry.
        m_Tracked.erase(m_Tracked.begin() + (t - m_Tracked.data()));
        t = nullptr;
    }

    BuffResult br;
    if (!t && e.Components.HasBuffs() && (known.hasMod || (s.detectByBuff && now >= known.nextBuffCheck))) {
        known.nextBuffCheck = now + kBuffRecheckSeconds;
        br = ReadBuffs(ctx, e.Components.Buffs);
    }
    const bool detected = known.hasMod || (s.detectByBuff && br.lightless);
    if (!t && !detected) return;

    if (!t) {
        m_Tracked.emplace_back();
        t = &m_Tracked.back();
        t->id = e.Id;
        t->path = Narrow(e.Path);
        t->debugMods = known.debugMods;
        t->nextBuffRefresh = now + kTrackedBuffSeconds;
        if (s.logDetections) {
            char msg[640];
            std::snprintf(msg, sizeof(msg), "Amanamu Void Alert: found id=%u by %s, %s", e.Id,
                          known.hasMod ? "monster mod" : "Lightless Well buff", t->path.c_str());
            ctx.Log.Info(msg);
        }
    }
    t->address = e.Address;
    t->buffsAddress = e.Components.Buffs;
    t->x = e.WorldX;
    t->y = e.WorldY;
    t->z = e.WorldZ;
    t->height = std::clamp(e.ModelBoundsZ, 60.0f, 400.0f);
    t->distance = distance;
    t->lastInRange = now;
    t->lastLive = now;
    t->byMod |= known.hasMod;
    t->byBuff |= br.lightless;
    if (br.scanned) {
        t->insideCloud = br.inside;
        t->debugBuffs = std::move(br.relevant);
    }
}

void Tracker::RefreshTracked(const PluginSDK::Context& ctx, const Settings& s, double now) {
    if (m_Suppressed || m_Tracked.empty()) return;
    m_Player = ctx.Entities.GetPlayer();
    m_HasPlayer = m_Player.Address != 0;

    for (Tracked& t : m_Tracked) {
        std::optional<PluginSDK::Entity> live = ctx.Entities.FindById(t.id);
        if (live && live->IsValid && live->Address == t.address) {
            if (IsDead(*live)) {
                t.lastLive = -1.0e9;   // pruned below
                continue;
            }
            t.x = live->WorldX;
            t.y = live->WorldY;
            t.z = live->WorldZ;
            if (live->ModelBoundsZ > 0.0f) t.height = std::clamp(live->ModelBoundsZ, 60.0f, 400.0f);
            t.lastLive = now;
            t.buffsAddress = live->Components.Buffs;
        }
        if (t.buffsAddress && now >= t.nextBuffRefresh && now - t.lastLive < 0.05) {
            t.nextBuffRefresh = now + kTrackedBuffSeconds;
            BuffResult br = ReadBuffs(ctx, t.buffsAddress);
            t.insideCloud = br.inside;
            t.byBuff |= br.lightless;
            t.debugBuffs = std::move(br.relevant);
        }
        if (m_HasPlayer) t.distance = Distance2D(t.x, t.y, m_Player.WorldX, m_Player.WorldY);
    }

    m_Tracked.erase(std::remove_if(m_Tracked.begin(), m_Tracked.end(), [&](const Tracked& t) {
        return now - t.lastInRange > s.keepOutOfRangeSeconds || now - t.lastLive > s.forgetVanishedSeconds;
    }), m_Tracked.end());

    if (m_Tracked.size() > kMaxTracked) {
        std::sort(m_Tracked.begin(), m_Tracked.end(),
                  [](const Tracked& a, const Tracked& b) { return a.distance < b.distance; });
        m_Tracked.resize(kMaxTracked);
    }
}

} // namespace AmanamuVoid
