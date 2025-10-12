# AtmosViz Basic Design

## Purpose
AtmosViz visualises Dolby Atmos speaker layouts for immersive mix monitoring. The tool focuses on presenting spatial energy in two perspectives:
- **Outside view** – orthographic projection outside the room for engineering checks.
- **Inside view** – perspective projection from the reference listening position to emulate the mixer’s vantage point.

## Major Subsystems
1. **Audio Processor (`AtmosVizAudioProcessor`)**
   - Receives per-speaker level data and frequency band splits.
   - Aggregates running metrics used by the visualiser (RMS, peak, band energy).
   - Exposes parameters for zoom, draw scale, view mode, heatmap density, and colour weights.
2. **Editor (`AtmosVizAudioProcessorEditor`)**
   - Hosts all UI controls (view mode selector, sliders, colour mix pad button) and synchronises them with processor parameters.
   - Manages responsive layout and control visibility (e.g., heatmap density slider appears only in Radiation Heatmap mode).
3. **Speaker Visualiser (`SpeakerVisualizerComponent`)**
   - Renders room geometry, speakers, trails, and heatmaps.
   - Encapsulates camera handling (inside/outside presets, zoom clamping, projection matrices).
   - Generates the colour legend content and notifies the editor of zoom changes.
4. **Colour & Legend Components**
   - `ColourLegendComponent` displays gradient bars for energy mapping.
   - `ColourMixPadComponent` call-out allows interactive balancing of low/mid/high emphasis.

## Data Flow
```
Audio Input → AtmosVizAudioProcessor (metrics & bands) → SpeakerVisualizerComponent
          ↘ UI Parameters (ValueTree & slider bindings) ↗ AtmosVizAudioProcessorEditor
```

## Camera & Projection Overview
- Outside view uses an orthographic projection with fixed room scaling.
- Inside view uses perspective matrices derived from `computeInsideProjectionParameters()` to achieve a natural first-person look. Camera presets share orientation defaults:
  - Home/User default zoom: 0.60 (clamped to ≥0.40)
  - Top default zoom: 0.25 (minimum 0.25)
  - Outside presets default zoom: 1.0 with 0.10–2.0 slider range.

## Visual Layers
1. **Room Wireframe** – axis-aligned box (Outside) or perspective lines (Inside) with near-plane clipping.
2. **Speakers** – icons positioned from Dolby Atmos definitions; colours derived from band weights.
3. **Overlays** – trails, heatmap quads, or activity glyphs depending on mode.
4. **HUD Elements** – legends, dividers, and scalar labels drawn by the editor.

## External Dependencies
- JUCE 8.0.1 (GUI, audio plug-in framework).
- Toolchain: MSVC 14.41 (Visual Studio 2022), C++17.
- Optional: `build_vst3.ps1` PowerShell script for automated x64 builds and bundle assembly.

## Persistence & State
- UI state is stored in the processor’s `AudioProcessorValueTreeState` and serialised with DAW sessions.
- User camera presets capture yaw, pitch, distance, and zoom; they are restored on parameter recall.

## Testing Strategy (Current)
- Manual verification using the standalone executable.
- Host smoke tests in a VST3-compatible DAW for automation, parameter recall, and re-scan behaviour.
- Visual regression checklists stored in `docs/work_log.md` for future automation.
