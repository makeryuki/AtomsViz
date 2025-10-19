# AtmosViz

AtmosViz is a JUCE-based Dolby Atmos visualisation plug-in and standalone tool. It renders speaker activity and heatmap energy distributions in both interior and exterior perspectives, exposing fine-grained UI controls for engineers who need to monitor immersive mixes.

**Limitation:** AtmosViz currently supports only 7.1.4 channel layouts. Other bus configurations are not yet supported.

## Highlights
- **Inside / Outside perspective suite**
  - Eight camera presets per perspective (Home, Front, Back, Left, Right, Top, User) with sensible defaults for yaw, pitch, and distance.
  - Inside presets lock the camera to the listening position, enforcing minimum zooms (40% for Home/User/Front/Back/Left/Right, 25% for Top) so the room never collapses.
  - Outside presets keep an orthographic cube to inspect speaker placement without perspective distortion.
- **Radiation Heatmap toolkit**
  - Density slider (Coarse to Ultra) reshapes the sampling grid from 5x5x3 up to 13x13x11 cells.
  - Inline legend explains the blue->green->amber->red intensity ramp and updates instantly with the selected mode.
  - Energy smoothing retains responsiveness while avoiding frame-to-frame flicker in quiet passages.
- **Colour mixing controls**
  - Low/Mid/High sliders expose precise weighting for frequency emphasis.
  - Colour Mix Pad call-out mirrors the sliders with a barycentric triangle UI so you can balance the palette visually.
  - Legend and speaker glyphs stay synchronised with weight changes for immediate feedback.
- **Temporal analytics suite**
  - Temporal Trails render motion paths with fading opacity to inspect object movement.
  - Peak Hold view highlights the latest per-speaker peak for troubleshooting underused channels.
  - Optional draw-scale mode adjusts glyph size independently from zoom to keep dense scenes readable.
- **Automation and host integration**
  - All major controls are backed by `AudioProcessorValueTreeState` parameters for DAW automation and recall.
  - Standalone executable mirrors the plug-in UI, enabling quick smoke tests without launching a host.

## Repository Layout
- `Source/` - plug-in and editor implementation.
- `JuceLibraryCode/` - auto-generated JUCE wrappers.
- `Builds/VisualStudio2022/` - generated Visual Studio projects (VST3, Standalone, helper).
- `dist/AtmosViz_v0.3.0_Windows_VST3.zip` - VST3 bundle ready for distribution.
- `dist/AtmosViz_v0.3.0_Windows_Standalone.zip` - portable standalone executable.
- `dist/AtmosViz_v0.3.0_Windows_CLAP.clap` - CLAP plug-in built via clap-juce-extensions.
- `docs/` - up-to-date design notes and user/developer documentation (see below).

## Building
1. Install **Visual Studio 2022** with the Desktop development with C++ workload.
2. Install JUCE 8.0.1 (path referenced in the projects: `C:\\juce-8.0.1-windows\\JUCE`).
3. From the repository root, run:
   ```powershell
   pwsh -File.\build_vst3.ps1 -Configuration Release
   ```
   This produces:
   - VST3 bundle at `Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3`
   - Standalone binary at `Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe`

> **Note:** The JUCE VST3 manifest helper currently exits with `MSB3073`. The plug-in is still built correctly; the post-build hook can be disabled if the manifest is not required.

## Installing the Plug-in
- Copy the entire folder `Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3` into `C:\\Program Files\\Common Files\\VST3`. Only the file `Contents\\x86_64-win\\AtmosViz.vst3` is required by most hosts.
- Alternatively, unpack `dist/AtmosViz_v0.3.0_Windows_VST3.zip` and drop the extracted bundle into your VST3 directory.
- Re-scan plug-ins in the host DAW after copying.

## Running the Standalone
- Extract `dist/AtmosViz_v0.3.0_Windows_Standalone.zip`.
- Run `AtmosViz.exe`; no installation is required, but Windows may prompt for confirmation on first launch.

## Installing the CLAP Plug-in
- Copy `dist/AtmosViz_v0.3.0_Windows_CLAP.clap` (or the `build-clap` artefact) into `C:/Program Files/Common Files/CLAP`.
- If your host scans a custom CLAP directory, place the `.clap` file there instead.
- No additional resources are required; the `.clap` bundle already contains the binaries generated via clap-juce-extensions.

## Documentation
- [User Manual](docs/user_manual.md) - step-by-step installation and UI walkthrough
The `docs/` directory contains the following refreshed documents:
- `basic_design.md` - high-level architecture overview.
- `detailed_design.md` - component responsibilities and rendering pipeline details.
- `requirements.md` - functional and non-functional requirements tracked against the current build.
- `user_manual.md` - installation, UI walkthrough, preset definitions, and troubleshooting.
- `work_log.md` - dated log of key changes and outstanding follow-up tasks.
- `visualization_reference.md` - colour schemes, heatmap density scale, and camera preset matrix.

For release packaging or regression planning, consult `docs/developer_guide.md` and `docs/release_checklist.md`.

## Release Workflow
1. Run the Release build script (see above).
2. Zip the resulting VST3 bundle as `dist/AtmosViz_v<version>_Windows_VST3.zip`.
3. Tag the commit (`git tag v<version>`), push (`git push && git push --tags`).
4. Create a GitHub Release and attach the zip. Include a short changelog excerpt from `docs/work_log.md`.
5. Smoke-test the plug-in in at least one host before publishing.

## Support & Next Steps
- Verify Inside presets after any camera math change; Outside must remain axis-aligned.
- Upcoming tasks (as of 2025-10-12): adjust Inside Top orientation, verify Radiation Heatmap column coverage.
- For issues or feature proposals, file GitHub Issues with screenshots and host details.

---
This software is being developed via pair programming with Codex (GPT-5).













