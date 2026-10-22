// Amanamu Void Alert: marks Amanamu (Lightless Well) abyss monsters with a ring,
// a label and an edge arrow whose colour shows whether the monster is still in
// its void cloud.
//
// Originally created by 1k4ru5g3 (https://github.com/1k4ru5g3/AmanamuVoidAlertPlugin).
// Maintained by POEFixer since October 2026.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "../../../POEFixer/plugin_sdk/PluginSDK.h"
#include "../../../POEFixer/imgui/imgui.h"
#include "../../../POEFixer/imgui/imgui_internal.h"   // ImTextInitClassifiers

#include "Overlay.h"
#include "Settings.h"
#include "SettingsUi.h"
#include "Tracker.h"

#include <chrono>
#include <exception>
#include <new>
#include <string>

// Plugin info: the host checks it before CreatePlugin; the publish tool reads it.
POEFIXER_PLUGIN_INFO(
    "AmanamuVoidAlert",
    "Amanamu Void Alert",
    "1.0.2",
    "1k4ru5g3, POEFixer",
    "Marks Amanamu (Lightless Well) abyss monsters with a ring, a label and an edge arrow whose colour shows whether the monster is still in its void cloud.")

namespace {

using namespace AmanamuVoid;

class AmanamuVoidAlertPlugin final : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Amanamu Void Alert"; }

    void OnEnable(bool /*isGameAttached*/) override {
        BindImGui();
        LoadSettings();
        m_Tracker.Reset();
        m_Start = std::chrono::steady_clock::now();
        m_Enabled = true;
    }

    void OnDisable() override {
        SaveSettings();
        m_Tracker.Reset();
        m_ShowDebugWindow = false;
        m_Enabled = false;
    }

    bool WantsOverlay() const override { return true; }

    void DrawUI() override {
        if (!m_Enabled || !HostCompatible() || !BindImGui()) return;
        RetryBlockedLoad();
        const PluginSDK::Context& c = *ctx();
        if (!c.Game.IsAttached() || !c.Game.IsInGame()) {
            m_Tracker.Reset();   // cheap when already empty; area data never survives a loading screen
            return;
        }
        const double now = Now();
        m_Tracker.Tick(c, m_Settings, now);

        const bool hidden = (m_Settings.onlyWhenForeground && !c.Game.IsForeground()) ||
                            (m_Settings.hideWhenMenuVisible && c.Game.IsMenuVisible());
        if (!hidden) DrawOverlay(c, m_Settings, m_Tracker, m_OverlayStats);
        if (m_ShowDebugWindow) DrawDebugWindow(m_Tracker, &m_ShowDebugWindow, now);
    }

    void DrawSettings() override {
        if (!BindImGui()) return;
        RetryBlockedLoad();
        PanelContext pc{ m_Settings, m_Tracker, m_OverlayStats, m_ShowDebugWindow, HostCompatible() };
        if (DrawSettingsPanel(pc)) m_Dirty = true;
        // Save once the edit is finished, not on every frame of a slider drag.
        if (m_Dirty && !ImGui::IsAnyItemActive()) {
            SaveSettings();
            m_Dirty = false;
        }
    }

    void SaveSettings() override {
        if (!m_Loaded) return;   // never overwrite the file with defaults before it was read
        try {
            m_Settings.Clamp();
            std::string json = ToJson(m_Settings);
            if (json == m_LastSaved) return;
            // The file could not be read at load: the player changed a setting
            // since, so it is replaced now, after keeping a copy of the old one.
            if (m_LoadBlocked) {
                m_LoadRetriesLeft = 0;   // the player's edit wins over a later read
                KeepUnreadableCopy(DirectoryPath());
            }
            if (Save(DirectoryPath(), json)) {
                m_LastSaved = std::move(json);
                if (m_LoadBlocked) {
                    m_LoadBlocked = false;
                    ctx()->Log.Warn("Amanamu Void Alert: replaced the unreadable AmanamuVoidAlert.json after a settings "
                                    "change (a copy was kept as .unreadable where possible)");
                }
            }
        } catch (const std::exception&) {
            // Out of memory while serialising: keep the previous file.
        }
    }

private:
    bool BindImGui() const {
        if (ctx()->ImGuiContext) ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));
        // ImGui 1.93 keeps its word-wrap character table in a static of each ImGui
        // copy, filled only when that copy builds a font atlas. This DLL never
        // does (it draws with the host's context), so without this every wrapped
        // text and tooltip breaks mid-word. Cheap: returns at once when filled.
        ImTextInitClassifiers();
        return ImGui::GetCurrentContext() != nullptr;
    }

    double Now() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_Start).count();
    }

    void LoadSettings() {
        m_LoadBlocked = false;
        m_LoadRetriesLeft = kLoadRetries;
        m_LoadRetryAt = std::chrono::steady_clock::now() + kLoadRetryEvery;
        try {
            Settings loaded;
            const LoadSource src = Load(DirectoryPath(), loaded);
            AdoptLoaded(src, std::move(loaded));
        } catch (const std::exception&) {
            m_Settings = Settings{};
            m_Loaded = true;
        }
    }

    void AdoptLoaded(LoadSource src, Settings&& loaded) {
        m_Settings = std::move(loaded);
        m_Loaded = true;
        if (src == LoadSource::Json) {
            m_LastSaved = ToJson(m_Settings);
        } else if (src == LoadSource::Unreadable) {
            // Defaults in memory, but nothing is written until a setting changes:
            // the file is the player's and is read again shortly.
            m_LoadBlocked = true;
            m_LastSaved = ToJson(m_Settings);
            ctx()->Log.Warn("Amanamu Void Alert: AmanamuVoidAlert.json exists but could not be read (another program "
                            "may hold it); using defaults without overwriting it, reading it again shortly");
        } else {
            m_LastSaved.clear();
            if (src == LoadSource::Legacy)
                ctx()->Log.Info("Amanamu Void Alert: settings moved from settings.txt to AmanamuVoidAlert.json");
            else if (src == LoadSource::CorruptJson)
                ctx()->Log.Warn("Amanamu Void Alert: AmanamuVoidAlert.json was unreadable; kept as .corrupt, defaults used");
            SaveSettings();
        }
    }

    // A settings file that was locked at load is read again a few times; the
    // read is a small file every few seconds, and stops once a setting changes.
    void RetryBlockedLoad() {
        if (!m_LoadBlocked || m_LoadRetriesLeft <= 0 || m_Dirty) return;
        const auto now = std::chrono::steady_clock::now();
        if (now < m_LoadRetryAt) return;
        m_LoadRetryAt = now + kLoadRetryEvery;
        --m_LoadRetriesLeft;
        try {
            Settings loaded;
            const LoadSource src = Load(DirectoryPath(), loaded);
            if (src == LoadSource::Unreadable) {
                if (m_LoadRetriesLeft == 0)
                    ctx()->Log.Warn("Amanamu Void Alert: AmanamuVoidAlert.json is still unreadable; defaults stay in "
                                    "use and the file is replaced only if a setting is changed");
                return;
            }
            m_LoadBlocked = false;
            AdoptLoaded(src, std::move(loaded));
            if (src == LoadSource::Json)
                ctx()->Log.Info("Amanamu Void Alert: AmanamuVoidAlert.json read on a later attempt");
        } catch (const std::exception&) {
            // Out of memory: keep the defaults and try again later.
        }
    }

    static constexpr int kLoadRetries = 15;
    static constexpr std::chrono::seconds kLoadRetryEvery{ 2 };

    Settings     m_Settings;
    Tracker      m_Tracker;
    OverlayStats m_OverlayStats;
    std::chrono::steady_clock::time_point m_Start = std::chrono::steady_clock::now();
    std::string  m_LastSaved;
    bool         m_Loaded = false;
    bool         m_LoadBlocked = false;   // the file exists but was unreadable at load
    int          m_LoadRetriesLeft = 0;
    std::chrono::steady_clock::time_point m_LoadRetryAt{};
    bool         m_Dirty = false;
    bool         m_Enabled = false;
    bool         m_ShowDebugWindow = false;
};

} // namespace

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() {
    return new (std::nothrow) AmanamuVoidAlertPlugin();
}

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* plugin) {
    delete plugin;
}
