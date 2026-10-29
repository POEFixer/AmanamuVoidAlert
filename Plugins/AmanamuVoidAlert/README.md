# Amanamu Void Alert

A POEFixer plugin for Path of Exile 2 abyss encounters. It marks Amanamu's Void monsters, the ones that
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
- **Also find by Amanamu's Void buff**: also track monsters that carry the Amanamu's Void buff, even when their
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

- A monster is recognised by the Lightless faction monster modifier and the `abyss_lightless_well` buffs. If a game
  patch renames them, the plugin needs an update.
- The cloud state comes from the monster's `abyss_lightless_well_immune` buff and is refreshed ten times a
  second.
- Settings live in `config/AmanamuVoidAlert.json` next to the plugin. An older `config/settings.txt` is read
  once and moved to the new file automatically.

## Credits

Originally created by 1k4ru5g3 (https://github.com/1k4ru5g3/AmanamuVoidAlertPlugin). Maintained by POEFixer
since October 2026.
