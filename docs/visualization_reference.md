# Visualization Reference

## View Modes
| Mode | Description | Notes |
|------|-------------|-------|
| Directional Lobes | Displays speaker coverage lobes as wireframe fans emanating from each loudspeaker. | Good for checking aim of static beds; colours follow band weights. |
| Layered Lobes | Stacks semi-transparent lobes by frequency band to emphasise Low / Mid / High balances. | Layer opacities reflect current band weight sliders. |
| Directivity Balloon | Renders a composite balloon around each speaker showing summed energy. | Use draw-scale mode to prevent overlap in dense scenes. |
| Radiation Heatmap | 3D grid sampling of room energy rendered as coloured quads. | Density slider (levels 1-5) changes sampling resolution; legend shows intensity ramp. |
| Temporal Trails | Draws fading paths for moving speakers/objects, colour-coded by band energy. | Trail length derived from `trailBufferCapacity`; decay tuned for ~2 s visibility. |

## Heatmap Density Mapping
| Level | Label | Grid (Depth x Width x Height) |
|-------|-------|--------------------------------|
| 1 | Coarse | 5 x 5 x 3 |
| 2 | Low | 7 x 7 x 5 |
| 3 | Medium | 9 x 9 x 7 |
| 4 | High | 11 x 11 x 9 |
| 5 | Ultra | 13 x 13 x 11 |

## Colour Legend Stops (Heatmap)
| Position | Colour | Meaning |
|----------|--------|---------|
| 0.00 | Blue (#1E3A8A) | Minimum energy |
| 0.25 | Teal (#0EA5E9) | Low energy |
| 0.50 | Green (#22C55E) | Moderate energy |
| 0.75 | Amber (#F59E0B) | Elevated energy |
| 1.00 | Red (#F87171) | Peak energy |

## Band Colour Weights
- Default slider values: Low 0.33, Mid 0.33, High 0.34.
- Slider range: 0.0 - 1.5 per band, automatically normalised when computing the final colour mix.
- Colour Mix Pad corners:
  - **Low (Blue)** - bottom-left node.
  - **Mid (Green)** - bottom-right node.
  - **High (Red)** - top node.
- The pad keeps the sum of weights at 1.0 by rebalancing the remaining nodes when one node moves.

## Camera Presets
| Group | Preset | Yaw | Pitch | Distance | Default Zoom |
|-------|--------|-----|-------|----------|--------------|
| Inside | Home | -95 deg | -5 deg | insideHomeBaseDistance | 0.60 |
| Inside | User | Restored from saved state | varies | stored | 0.60 |
| Inside | Top | 180 deg | -90 deg | insideTopDistance | 0.25 |
| Inside | Front | -90 deg | 0 deg | insideDefaultDistance | 0.60 |
| Inside | Back | 90 deg | 0 deg | insideDefaultDistance | 0.60 |
| Inside | Left | 180 deg | 0 deg | insideDefaultDistance | 0.60 |
| Inside | Right | 0 deg | 0 deg | insideDefaultDistance | 0.60 |
| Outside | Home | -60 deg | 15 deg | outsideDefaultDistance | 1.00 |
| Outside | Front/Back/Left/Right/Top | Axis-aligned yaw/pitch pairs | outsideDefaultDistance | 1.00 |

## Divider & Layout Guidelines
- Header uses an 8 px spacing increment and adds dividers after heatmap controls and the band weight block.
- Minimum window width for full header layout: 980 px.
- Zoom slider label always reads "Zoom (10%-200%)" even when slider mode changes; draw-scale mode updates the tooltip only.

## QA Checklist Snapshot
1. Confirm all six Inside presets respect minimum zoom limits.
2. Ensure Outside presets remain orthographic and axis-aligned.
3. Heatmap density label + slider + value label align right under the View Mode combo (<1 px jitter allowed).
4. Colour Mix Pad button opens a floating call-out; closing the pad restores keyboard focus to the main editor.
5. Legend updates when switching modes; Heatmap label reads "Heatmap Intensity".
