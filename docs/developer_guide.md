# Developer Guide

## Environment Setup
1. Install Visual Studio 2022 (17.11+) with C++ Desktop workload.
2. Install CMake tools (optional) for auxiliary utilities.
3. Install JUCE 8.0.1 and update `JuceLibraryCode/JuceHeader.h` include paths if JUCE is stored elsewhere.
4. Clone the repository and initialise subdirectories:
   ```powershell
   git clone https://github.com/makeryuki/AtomsViz.git
   cd AtmosViz
   ```

## Building
- **Debug build:** `pwsh -File .\build_vst3.ps1`
- **Release build:** `pwsh -File .\build_vst3.ps1 -Configuration Release`
- The script selects the first available `MSBuild.exe` amd64 path. Update `build_vst3.ps1` if Visual Studio is installed in a different SKU.
- Artifacts:
  - `Builds/VisualStudio2022/x64/<Config>/VST3/AtmosViz.vst3`
  - `Builds/VisualStudio2022/x64/<Config>/Standalone Plugin/AtmosViz.exe`

## Debugging Tips
- Launch the standalone exe for rapid iteration; attach debugger via Visual Studio if needed.
- Use JUCE’s Projucer for component layout experiments, but keep source of truth in `Source/`.
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
- `Compress-Archive` call to create `dist/AtmosViz_v<version>_Windows_VST3.zip`
- Git tag + GitHub Release creation

## Testing Strategy (Current)
- **Visual:** Manual pass covering all camera presets and view modes (use `1/` screenshot set).
- **Audio:** Feed pink noise into Atmos buses; verify colour responses and heatmap behaviour.
- **Regression:** Compare new screenshots against archived references when changing projection math.

## Future Enhancements
- Add unit tests for `computeInsideProjectionParameters`.
- Integrate automated screenshot capture using JUCE’s `Image` class for diffing.
- Investigate manifest helper fix to remove post-build warnings.
