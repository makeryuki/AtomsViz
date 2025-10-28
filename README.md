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
  - Colours follow a blue -> green -> amber -> red ramp that mirrors the per-frame energy range (see `docs/visualization_reference.md`).
  - Energy smoothing retains responsiveness while avoiding frame-to-frame flicker in quiet passages.
- **Colour mixing & scaling controls**
  - Low/Mid/High sliders expose precise weighting for frequency emphasis.
  - Colour Mix Pad call-out mirrors the sliders with a barycentric triangle UI so you can balance the palette visually.
  - New *Visualization Gain* slider adjusts lobe and glyph reach independently from camera zoom, replacing the legacy legend framing.
- **Temporal analytics suite**
  - Temporal Trails render motion paths with fading opacity to inspect object movement.
  - Peak Hold view highlights the latest per-speaker peak for troubleshooting underused channels.
  - Optional draw-scale mode adjusts glyph size independently from zoom to keep dense scenes readable.
- **Channel mapping safeguards**
  - Uses JUCE channel-type metadata to pin each meter to the correct 7.1.4 speaker.
  - Unknown channels are ignored gracefully so visuals never drift when extra stems are present.
- **Automation and host integration**
  - All major controls are backed by `AudioProcessorValueTreeState` parameters for DAW automation and recall.
  - Standalone executable mirrors the plug-in UI, enabling quick smoke tests without launching a host.

## Repository Layout
- `Source/` - plug-in and editor implementation.
- `JuceLibraryCode/` - auto-generated JUCE wrappers.
- `Builds/VisualStudio2022/` - generated Visual Studio projects (VST3, Standalone, helper).
- `Builds/MacOSX/` - generated Xcode projects and build artefacts (AU, VST3, Standalone).
- `dist/AtmosViz_v0.5.0_Windows_VST3.zip` - VST3 bundle ready for distribution.
- `dist/AtmosViz_v0.5.0_Windows_Standalone.zip` - portable standalone executable.
- `dist/AtmosViz_v0.5.0_Windows_CLAP.clap` - CLAP plug-in built via clap-juce-extensions.
- `dist/AtmosViz_v0.5.0_macOS_VST3.zip` - macOS VST3 bundle (latest published macOS build).
- `dist/AtmosViz_v0.5.0_macOS_AU.zip` - macOS Audio Unit component.
- `dist/AtmosViz_v0.5.0_macOS_Standalone.zip` - macOS standalone `.app` bundle.
- `dist/AtmosViz_v0.5.0_macOS_CLAP.clap` - macOS CLAP plug-in bundle.
- `docs/` - up-to-date design notes and user/developer documentation (see below).

## Building

### Windows
1. Install **Visual Studio 2022** with the Desktop development with C++ workload.
2. Install JUCE 8.0.1 (path referenced in the projects: `C:\\juce-8.0.1-windows\\JUCE`).
3. From the repository root, run:
   ```powershell
   pwsh -File .\build_vst3.ps1 -Configuration Release
   ```
   This produces:
   - VST3 bundle at `Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3`
   - Standalone binary at `Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe`

> **Note:** The JUCE VST3 manifest helper currently exits with `MSB3073`. The plug-in is still built correctly; the post-build hook can be disabled if the manifest is not required.

### macOS
1. Install **Xcode 15** (includes `xcodebuild`) plus the matching Command Line Tools.
2. Install JUCE 8.0.1 and Projucer (drag the JUCE folder to `/Applications/JUCE` or clone to a local path). Update `AtmosViz.jucer` module search paths if JUCE lives elsewhere.
3. Regenerate the Xcode project:
   ```bash
   /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave AtmosViz.jucer
   ```
4. Build the plug-in formats:
   ```bash
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - AU" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - VST3" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - Standalone Plugin" -configuration Release build
   ```
   Release artefacts land in `Builds/MacOSX/build/Release/` (`.component`, `.vst3`, `.app`).
5. Build the CLAP bundle via `clap-juce-extensions`:
   ```bash
   cmake -B build-clap-mac -S . -G "Xcode" -DJUCER_GENERATOR=Xcode -DPATH_TO_JUCE=/Applications/JUCE
   cmake --build build-clap-mac --config Release
   ```
   The finished bundle is at `build-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap`.

## Installing the Plug-in
- **Windows (VST3):** copy `Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3` (or unzip `dist/AtmosViz_v0.5.0_Windows_VST3.zip`) into `C:\Program Files\Common Files\VST3`.
- **macOS (VST3):** copy `Builds/MacOSX/build/Release/AtmosViz.vst3` (or unzip `dist/AtmosViz_v0.5.0_macOS_VST3.zip`) into `/Library/Audio/Plug-Ins/VST3/`.
- **macOS (AU):** copy `Builds/MacOSX/build/Release/AtmosViz.component` (or unzip `dist/AtmosViz_v0.5.0_macOS_AU.zip`) into `/Library/Audio/Plug-Ins/Components/`.
- Re-scan plug-ins in the host DAW after copying.

## Running the Standalone
- **Windows:** extract `dist/AtmosViz_v0.5.0_Windows_Standalone.zip` and run `AtmosViz.exe`.
- **macOS:** unzip `dist/AtmosViz_v0.5.0_macOS_Standalone.zip`, drag `AtmosViz.app` to `/Applications` (or run in-place), then launch via Finder. macOS Gatekeeper may prompt on first launch.

## Installing the CLAP Plug-in
- **Windows:** copy `dist/AtmosViz_v0.5.0_Windows_CLAP.clap` (or the `build-clap` artefact) into `C:/Program Files/Common Files/CLAP`.
- **macOS:** copy `dist/AtmosViz_v0.5.0_macOS_CLAP.clap` (or `build-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap`) into `/Library/Audio/Plug-Ins/CLAP/`.
- If your host scans custom folders, place the `.clap` bundle in the appropriate directory and trigger a plug-in rescan.

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
- Upcoming tasks (snapshot): adjust Inside Top orientation and verify Radiation Heatmap column coverage.
- For issues or feature proposals, file GitHub Issues with screenshots and host details.

---
This software is being developed via pair programming with Codex (GPT-5).

















