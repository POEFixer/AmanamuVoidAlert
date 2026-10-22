# Amanamu Void Alert

Current version: 1.0.2

Requires POE2Fixer 364 or newer.

A POEFixer plugin for Path of Exile 2 abyss encounters. It marks Amanamu (Lightless Well) monsters, the ones that
sit in a void cloud, with a ring and a label, and points to them with an arrow at the screen edge when they are
off screen. The colour tells you at a glance whether the monster is still in its cloud or has left it.

## How to use

1. Install **Amanamu Void Alert** from the **Plugin Manager**, the first tab under POEFixer's **Plugins** tab. It is
   enabled right away, and the Plugin Manager keeps it up to date. The **Enabled** box at the top of the
   plugin's own tab turns it off and on.
2. Play a map with abyss content. When an Amanamu monster comes within range it gets a ring and the label
   "AMANAMU VOID" with its cloud state; off screen, an arrow at the screen edge points to it.
3. Purple (default) means the monster is in its cloud; green means it has left it.

## Settings

### Detection
- **Only rare and unique monsters**: ignore normal and magic monsters.
- **Also find by Lightless Well buff**: also track monsters that carry a Lightless Well buff, even when their
  modifier is not readable. Turn it off if other monsters standing in the cloud get marked too.
- **Tracking range**: how far from your character a monster is still picked up (game distance units).
- **Keep when out of range**: how long a monster stays marked after leaving the tracking range.
- **Forget when gone**: how long the marker stays at the last known spot after the monster disappears.

### When to show
- **Only while the game window is active**, **Hide while the POEFixer menu is open**. Never drawn in towns and
  hideouts, or while the game is paused.

### Marker on the monster
- **Ring**, **Name and cloud state**, **Show distance** (also on the edge arrow).
- **Ring size**, **Label height** (pixels at Scale 100%).

### Edge arrow
- **Point to monsters off screen**; **Also while the monster is on screen**.
- **Distance from the edge**: raise it if the arrow hides behind the game's own UI.

### Look
- Colours for **In its cloud** and **Left its cloud** (each with a sample of the in-game label), **Scale** (rings, arrows and text; also follows the
  POEFixer font size) and **Opacity**. **Default look** restores them.

### Diagnostics
Live numbers, **Show the debug window** (a list of tracked monsters with their modifiers and buffs; not saved),
**Write new finds to the POEFixer log**, and **Forget tracked monsters**.

## Known limits

- A monster is recognised by the Lightless faction monster modifier and the Lightless Well buffs. If a game
  patch renames them, the plugin needs an update.
- The cloud state comes from the monster's `abyss_lightless_well_immune` buff and is refreshed ten times a
  second.
- Settings live in `config/AmanamuVoidAlert.json` next to the plugin. An older `config/settings.txt` is read
  once and moved to the new file automatically.

## Install

Install it from POE2Fixer → Plugins → Plugin Manager (one click, updates included), or download `AmanamuVoidAlert.zip` from [Releases](https://github.com/POEFixer/AmanamuVoidAlert/releases/latest) and extract it into the POE2Fixer folder (it creates `Plugins\AmanamuVoidAlert\`). Then enable the plugin in the Plugins tab.

## Build from source

This repository is the plugin folder from the POE2Fixer source tree (`Plugins/AmanamuVoidAlert/`) together with the host files it compiles against, kept at the same relative paths:

- `POEFixer/plugin_sdk/` - the POE2Fixer Plugin SDK v6 headers (`PluginAbi.h`, `PluginSDK.h`)
- `POEFixer/imgui/` - the Dear ImGui sources the DLL compiles against

1. Install Visual Studio 2022 with the "Desktop development with C++" workload (MSVC v143, Windows 10/11 SDK).
2. Open `AmanamuVoidAlert.sln`, select **Release | x64** and build the solution.
3. The plugin is written to `x64\Release\Plugins\AmanamuVoidAlert\AmanamuVoidAlert.dll` (Release builds carry no debug information, see `Directory.Build.targets`).
4. Copy `AmanamuVoidAlert.dll` into `Plugins\AmanamuVoidAlert\` next to the POE2Fixer executable.

## Credits

Originally created by 1k4ru5g3 (https://github.com/1k4ru5g3/AmanamuVoidAlertPlugin). Maintained by POEFixer
since October 2026.
