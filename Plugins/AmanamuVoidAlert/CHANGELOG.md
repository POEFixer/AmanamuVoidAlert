# Changelog

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
