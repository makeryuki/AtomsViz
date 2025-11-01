# Channel Layout Expansion Plan

## Background
- Current builds only supported Dolby Atmos 7.1.4 (12 channels).
- JUCE's `AudioChannelSet` exposes additional immersive layouts (7.1.2, 7.1.6, 5.1.x, 9.1.x, etc.) that production users rely on.

## Phase Breakdown
1. **Foundation**: replace the fixed `std::array` containers with layout-dependent `std::vector`s and generate definitions per layout via `buildSpeakerDefinitions()`.
2. **7.x family**: add seeds for 7.1.2 / 7.1.6, adjust azimuth/elevation tables, and ensure preset zoom constraints remain sensible.
3. **5.x family**: implement 5.1 / 5.1.2 / 5.1.4. Non-present speakers should be hidden without leaving gaps.
4. **9.x and wider**: introduce wide and top-side speakers, rework label collision avoidance, and revisit camera presets for dense layouts.
5. **Generic layouts**: handle `AudioChannelSet::discreteChannels()` by allowing externally supplied metadata for position/labels.

## Test Considerations
- `AudioChannelSet` channel types must map 1:1 onto definition seeds.
- `processBlock` skips channels with no known definition without corrupting state.
- Inside/Outside labels remain legible and non-overlapping.
- Heatmap / Trails / Peak Hold render correctly irrespective of speaker count.
- Regression checks for legacy 7.1.4 sessions.

## Current Scope (targeting v0.6.0)
- Phase 1 foundation work.
- 7.x support for 7.1.2 and 7.1.6.
- Documentation refresh (README / user_manual / requirements).

## Progress (2025-10-29)
- Completed Phase 1 (dynamic containers) and added automatic detection for 7.1.2 / 7.1.4 / 7.1.6.
- Next steps: implement 5.x layouts and tune wide/top-side positioning for 9.x.
