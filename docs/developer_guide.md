# Developer Guide

日本語版: [developer_guide_ja.md](ja/developer_guide_ja.md)

## Environment Setup
1. Install Visual Studio 2022 (17.11+) with C++ Desktop workload (Windows).
2. Install Xcode 15 (or newer) and the matching Command Line Tools if you are building on macOS.
3. Install CMake 3.20+ (required for CLAP builds on all platforms).
4. Install JUCE 8.0.1 and update `AtmosViz.jucer` module search paths if JUCE is stored outside the default locations (`C:\juce-8.0.1-windows\JUCE` or `/Applications/JUCE`).
5. Clone the repository and initialise submodules:
   ```powershell
   git clone https://github.com/makeryuki/AtomsViz.git
   cd AtmosViz
   git submodule update --init --recursive
   ```

## Building

### Windows
- **Debug build:** `pwsh -File .\build_vst3.ps1`
- **Release build:** `pwsh -File .\build_vst3.ps1 -Configuration Release`
- The script selects the first available `MSBuild.exe` amd64 path. Update `build_vst3.ps1` if Visual Studio is installed in a different SKU.
- Artefacts:
  - `Builds/VisualStudio2022/x64/<Config>/VST3/AtmosViz.vst3`
  - `Builds/VisualStudio2022/x64/<Config>/Standalone Plugin/AtmosViz.exe`

### macOS
1. Regenerate the Xcode project after adjusting JUCE module paths:
   ```bash
   /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave AtmosViz.jucer
   ```
2. Build Release variants:
   ```bash
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - AU" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - VST3" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - Standalone Plugin" -configuration Release build
   ```
3. Artefacts land in `Builds/MacOSX/build/Release/` (`AtmosViz.component`, `AtmosViz.vst3`, `AtmosViz.app`).

### Building the CLAP plug-in
1. Ensure the platform-specific Release build has been produced (`Shared Code/AtmosViz.lib` on Windows, `Builds/MacOSX/build/Release/libAtmosViz.a` on macOS).
2. Configure the CMake wrapper around the Projucer project:
   - Windows:
     ```powershell
     cmake -B build-clap -S . -G "Visual Studio 17 2022"
     ```
   - macOS:
     ```bash
     cmake -B build-clap-mac -S . -G "Xcode" -DJUCER_GENERATOR=Xcode -DPATH_TO_JUCE=/Applications/JUCE
     ```
3. Build the Release configuration:
   - Windows: `cmake --build build-clap --config Release`
   - macOS: `cmake --build build-clap-mac --config Release`
4. The resulting plug-in bundle is emitted at `build-clap/AtmosViz_artefacts/Release/AtmosViz.clap` (Windows) or `build-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap` (macOS).

## Debugging Tips
- Launch the standalone exe for rapid iteration; attach debugger via Visual Studio if needed.
- Use JUCE's Projucer for component layout experiments, but keep source of truth in `Source/`.
- Enable JUCE assertions in Debug to catch camera math regressions.
- Add temporary `DBG` statements sparingly; remove before committing.

## Coding Guidelines
- C++17 only; avoid introducing compiler-specific extensions.
- Default to `juce::` utilities (Vectors, Colours) unless STL is more appropriate.
- Keep UI colour constants grouped near the top of `PluginEditor.cpp`.
- Maintain ASCII comments and docstrings to avoid encoding issues (`C4819` warnings stem from third-party headers only).

## Documentation Workflow
1. Archive current docs under `docs/_archive/<yyyyMMdd>/` before major rewrites.
2. Update `README.md` plus the relevant docs (`basic_design`, `detailed_design`, `user_manual`, etc.).
3. Record noteworthy changes in `docs/work_log.md` with absolute dates.
4. Cross-reference outstanding TODOs in `requirements.md`.

## Release Packaging
See `docs/release_checklist.md` for the step-by-step routine. Key automation pieces:
- `build_vst3.ps1 -Configuration Release`
- `cmake --build build-clap --config Release`
- `Compress-Archive` calls to create distribution zips and copy the `.clap`
- Git tag + GitHub Release creation

## Testing Strategy (Current)
- **Visual:** Manual pass covering all camera presets and view modes (use `1/` screenshot set).
- **Audio:** Feed pink noise into Atmos buses; verify colour responses and heatmap behaviour.
- **Regression:** Compare new screenshots against archived references when changing projection math.

## Future Enhancements
- Add unit tests for `computeInsideProjectionParameters`.
- Integrate automated screenshot capture using JUCE's `Image` class for diffing.
- Investigate manifest helper fix to remove post-build warnings.


