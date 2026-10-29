# Changelog

## 1.0.4

- Follows the program language (ten languages).
- The buff option names the buff as the game does: "Amanamu's Void".

## 1.0.3

- Works with the separate menu window (POEFixer 366): the host hands the plugin the right drawing context for its settings tab and for its overlay.

## 1.0.2

- Rebuilt with the updated plugin SDK headers (POEFixer 365 adds optional calls for UI control names, stash objects and tabs, wider entity scans and the host automation flag). No change in what the plugin does.

## 1.0.1

- The "?" next to a section title now sits in its own gap instead of on the section's line.
- Diagnostics now looks the same as in the other POEFixer plugins.
- The bottom of the settings tab names the plugin's original author, as in the other plugins maintained by POEFixer.

## 1.0.0

- First release maintained by POEFixer.
- The label now sits above the monster and the ring around its body (before, both were drawn below its feet).
- The edge arrow now points the right way for monsters far off screen instead of falling back to an old
  direction.
- Much lighter on performance: monster modifiers are read once per monster and the search is spread over
  frames, so busy maps no longer slow the game down.
- New settings tab in the POEFixer style with explanations on every option, plus colour, scale and opacity
  settings. Text is outlined and sizes follow the POEFixer font size.
- The debug window no longer opens by default; it is in the Diagnostics section.
- Settings are saved safely and automatically; your old settings are taken over.
