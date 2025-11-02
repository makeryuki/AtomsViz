# AtmosViz Detailed Design

日本語版: [detailed_design_ja.md](ja/detailed_design_ja.md)

## UI Composition
- **Header Strip**
  - `visualizationCombo`: selects Radiation Heatmap, Temporal Trails, Peak Hold, etc.
  - `sliderModeCombo`: toggles Zoom vs Draw Scale slider behaviour.
  - `heatmapDensitySlider`: visible only in Radiation Heatmap mode (range 1–5, labelled Coarse → Ultra).
  - `bandWeightTitleLabel` and three rotary sliders (Low/Mid/High) mapped to `colourMixWeights` parameters.
  - `colourPadButton`: opens a `juce::CallOutBox` hosting `ColourMixPadComponent` for intuitive weight adjustment.
  - `colourLegend`: gradient bar with captions derived from the current visualisation mode.
- **Content Area**
  - `SpeakerVisualizerComponent` fills the remainder of the editor window, handling painting and mouse interactions.

## Parameter Binding
The editor creates `AudioProcessorValueTreeState::SliderAttachment` objects for all sliders. For combo boxes, it manually forwards changes to processor setters. The colour mix pad writes back into the same parameter triplet, ensuring consistency between sliders and pad.

## Camera Handling
- Camera presets are enumerated in `SpeakerVisualizerComponent::CameraPreset`. The editor presents them through a segmented button group.
- `applyZoomFactorToCamera()` clamps zoom via `getCurrentMinZoom()` which differentiates Inside vs Outside minima.
- `setCameraPreset()` stores per-preset yaw, pitch, and distance, then recalculates projection matrices.
- Inside projections use `computeInsideProjectionParameters()` to derive horizontal/vertical FOV, near plane, and focal lengths from room dimensions.

## Rendering Pipeline
1. **Data Preparation**
   - Metrics from the processor are copied into `speakers` array each frame.
   - Heatmap grid points are lazily regenerated when density changes.
   - Trails use a circular buffer with decaying alpha.
2. **Projection**
   - `projectPoint()` handles both inside (perspective) and outside (orthographic) cases.
   - Inside view normalises points against near plane; if a point is behind the camera (`depth < insideNearPlane`), it is skipped.
3. **Drawing**
   - Room lines are emitted by `drawRoomOutside()` or `drawRoomInside()`.
   - Speaker glyphs leverage `juce::Path` definitions with per-speaker colours.
   - Heatmap cells are blended quads with alpha scaled by normalised energy.
   - Legends and labels use JUCE `Graphics::drawFittedText` with enforced bounds to avoid the previous C2661 error path.

## Colour System
- Frequency bands (`low`, `mid`, `high`) are combined using weights stored in `bandColourWeights`.
- The Colour Mix Pad maps barycentric coordinates of the triangle UI into new weight triplets.
- Heatmap gradients are sampled via `colourForHeatmapRatio()` and mirrored in the legend component.

## Interaction Model
- Mouse wheel: adjusts zoom (respecting min/max) unless slider mode switches to Draw Scale.
- Left drag (inside view): rotates yaw/pitch, clamped to prevent gimbal lock.
- Right click: resets to User preset defaults.
- Call-out pad: `CallOutBox::launchAsynchronously` anchored to the Colour Mix Pad button.

## Persistence
- `getStateInformation` serialises the ValueTree, including latest camera preset IDs and colour weights.
- `setStateInformation` restores parameters, then calls `applyZoomFactorToCamera()` to re-sync the view.

## Error Handling & Logging
- Assertions guard invalid preset transitions and projection anomalies in debug builds.
- UI code avoids dereferencing null `visualizer` pointers by checking after `setVisualizer`.

## Known Limitations
- VST3 manifest helper remains disabled; no moduleinfo metadata is packaged.
- Heatmap calculation currently assumes Cartesian room layout; irregular rooms would need a new sampling grid.
- No automated tests; manual regression required after camera/projection changes.


