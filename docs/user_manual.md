# AtmosViz User Manual

**Limitation:** AtmosViz currently supports only 7.1.4 channel layouts. Other bus configurations are not yet supported.

## 1. Installation
1. Download the Release zip (`dist/AtmosViz_v0.3.0_Windows_VST3.zip`) from the repository or GitHub Release page.
2. Extract the archive; you will get `AtmosViz.vst3` (a folder bundle).
3. Copy the entire bundle into `C:\Program Files\Common Files\VST3`.
4. Launch your DAW and rescan plug-ins. AtmosViz appears under *Effects*.
5. For the standalone tool, run `Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe`.

## 2. Quick Start
1. Open AtmosViz on an Atmos-capable session.
2. Choose a **View Mode** from the drop-down:
   - *Radiation Heatmap*
   - *Temporal Trails*
   - *Peak Hold*
3. Pick a **Perspective** (Inside or Outside) and a camera preset button (Home, Front, Back, Left, Right, Top, User).
4. Use the **Zoom slider** (10%-200%). Inside presets honour minimum zooms (40% or 25%).
5. Adjust **Heatmap Density** (if visible) to refine spatial sampling.
6. Tweak **Band Colour Weights** or open the **Colour Mix Pad** to rebalance low/mid/high emphasis.

## 3. Controls Reference
| Control | Location | Description |
|---------|----------|-------------|
| View Mode combo | Header, left | Switches between visualisation algorithms. Heatmap reveals density slider and heatmap legend. |
| Slider Mode combo | Header | Chooses whether the horizontal slider controls Zoom or Draw Scale. |
| Zoom/Draw Scale slider | Header | Zoom: 10-200%. Draw Scale: scales speaker glyph size without moving the camera. |
| Heatmap Density slider | Header (Heatmap only) | Levels 1-5 labelled Coarse, Low, Medium, High, Ultra. Updates sampling grid and legend caption. |
| Band Colour sliders | Header | Three linear sliders (Low, Mid, High) that weight colour contribution. |
| Colour Mix Pad button | Header | Opens triangular pad; dragging any node rewrites band weights. |
| Colour Legend | Header, right | Shows gradient for current mode. For Heatmap: Low/Mid/High labels with colour ramp. |
| Camera preset buttons | Left of content | Instant view changes for Inside/Outside sets. User stores last manual orientation. |
| Reset button | Context menu | Right-click the visualiser to reset to User defaults. |

## 4. Camera Behaviour
- **Inside Home/User**: Camera fixed at room origin. Defaults yaw -95 deg, pitch -5 deg, zoom 60%. Wheel clamps to >=40%.
- **Inside Top**: Camera looks downward from origin. Default zoom 25%, min 25%.
- **Outside**: Orthographic orbit around the room bounding box; zoom independent of inside constraints.

## 5. Colour Mixing
- Colours derive from normalised band energy (Low, Mid, High) scaled by user-defined weights.
- Default weights favour balanced palette (Low=0.33, Mid=0.33, High=0.34).
- Colour Mix Pad anchors corners to pure Blue (Low), Green (Mid), Red (High). Interior mixes yield intermediary hues.

## 6. Heatmap Interpretation
- Heatmap intensity values are normalised to the peak of the current frame with smoothing to avoid flicker.
- Density level sets grid resolution (Depth x Width x Height):
  - 1: 5x5x3
  - 2: 7x7x5
  - 3: 9x9x7
  - 4: 11x11x9
  - 5: 13x13x11
- Legend labels map energy to colour gradient; low energy stays blue, mid energy green, high energy orange/red.

## 7. Troubleshooting
| Symptom | Remedy |
|---------|--------|
| Plug-in missing in DAW | Confirm the bundle sits in `\VST3\` and re-scan. If still missing, copy `AtmosViz.vst3` while DAW is closed. |
| Camera jumps unexpectedly | Check Slider Mode (Zoom vs Draw Scale). Reset by right-clicking the visualiser. |
| Colour Mix Pad does not open | Ensure the editor window has focus; the pad opens as a floating call-out anchored to the button. |
| Heatmap slider hidden | Only visible while *Radiation Heatmap* view mode is active. |
| Outside view distorted | Verify you are on the latest commit (`master`) and the preset buttons show "Outside" group. |

## 8. Support Channels
- File GitHub Issues with description, host, sample rate, and screenshot (use `1/` folder templates).

## 9. Future Roadmap (snapshot)
- Revisit Inside Top orientation (front wall alignment).
- Automate visual regression via scripted camera sweeps.
- Add optional 3D asset overlay for room treatment cues.









