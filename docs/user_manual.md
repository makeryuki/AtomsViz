# AtmosViz User Manual

**Limitation:** AtmosViz currently supports Dolby Atmos 7.1.2 / 7.1.4 / 7.1.6 channel layouts. Other bus configurations are not yet supported.

## 1. Installation
**Windows (VST3 + Standalone)**
1. Download `dist/AtmosViz_v0.5.0_Windows_VST3.zip`.
2. Extract; you will get `AtmosViz.vst3` (bundle folder).
3. Copy the bundle into `C:\Program Files\Common Files\VST3\`.
4. Launch your DAW and rescan plug-ins (most hosts list AtmosViz under *Effects*).
5. Extract `dist/AtmosViz_v0.5.0_Windows_Standalone.zip` and run `AtmosViz.exe` for the standalone tool.

**macOS (AU / VST3 / Standalone / CLAP)**
1. Download the macOS release zips currently published (`dist/AtmosViz_v0.5.0_macOS_AU.zip`, `dist/AtmosViz_v0.5.0_macOS_VST3.zip`, `dist/AtmosViz_v0.5.0_macOS_Standalone.zip`) and/or the CLAP bundle (`dist/AtmosViz_v0.5.0_macOS_CLAP.clap`).
2. Extract the relevant archive(s). Each plug-in is a bundle (`AtmosViz.component`, `AtmosViz.vst3`, `AtmosViz.clap`).
3. Copy bundles to the standard system locations:
   - AU: `/Library/Audio/Plug-Ins/Components/`
   - VST3: `/Library/Audio/Plug-Ins/VST3/`
   - CLAP: `/Library/Audio/Plug-Ins/CLAP/`
4. For the standalone tool, drag `AtmosViz.app` into `/Applications/` (or launch directly from the extracted folder). On first run, macOS Gatekeeper may prompt you to approve the binary (`System Settings > Privacy & Security`).
5. Rescan plug-ins in your host after copying. Some hosts (Logic Pro, MainStage) require a restart to validate new Audio Units.

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
7. Adjust the **Visualization Gain** slider to expand or shrink lobes without altering the camera zoom.

## 3. Controls Reference
| Control | Location | Description |
|---------|----------|-------------|
| View Mode combo | Header, left | Switches between visualisation algorithms. Heatmap reveals the density slider. |
| Slider Mode combo | Header | Chooses whether the horizontal slider controls Zoom or Draw Scale. |
| Zoom/Draw Scale slider | Header | Zoom: 10-200% with Inside minimums enforced. Draw Scale adjusts glyph reach independently of camera position. |
| Visualization Gain slider | Header | Scales lobes and trails between -50% and +100% relative to the baseline, independent of the zoom slider. |
| Heatmap Density slider | Header (Heatmap only) | Levels 1-5 (Coarse -> Ultra) adjusting sampling grid resolution. |
| Band Colour sliders | Header | Three linear Low/Mid/High controls that weight colour contribution. |
| Colour Mix Pad button | Header | Opens the triangular pad; dragging a node rewrites band weights. |
| Camera preset buttons | Header rows | Instant view changes for Inside/Outside sets. User stores last manual orientation. |
| Reset to User | Context menu | Right-click the visualiser to restore the stored User state. |

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
- Colours follow a blue -> green -> amber -> red gradient that maps the normalised energy range for the current frame.

## 7. Troubleshooting
| Symptom | Remedy |
|---------|--------|
| Plug-in missing in DAW | Confirm the bundle sits in `\VST3\` (Windows) or `/Library/Audio/Plug-Ins/{VST3,Components,CLAP}` (macOS) and re-scan while the host is closed. |
| macOS AU not validated | Open `System Settings ▸ Privacy & Security`, allow AtmosViz if Gatekeeper blocked it, then rerun the host so AU validation can complete. |
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














