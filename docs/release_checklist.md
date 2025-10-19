# Release Checklist

## Preparation
- [ ] Ensure `master` is up to date and all tests/manual checks have passed.
- [ ] Update `docs/work_log.md` with release summary.
- [ ] Confirm `requirements.md` statuses are accurate.

## Build
- [ ] Run `pwsh -File .\build_vst3.ps1 -Configuration Release`.
- [ ] Run `cmake --build build-clap --config Release` (after `cmake -B build-clap -S .` if not yet configured).
- [ ] Verify artifacts:
  - [ ] `Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3`
  - [ ] `build-clap/AtmosViz_artefacts/Release/AtmosViz.clap`
  - [ ] `Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe`
- [ ] Confirm Heatmap, Temporal Trails, Peak Hold views render correctly in Release build.

## Packaging
- [ ] Create distribution bundles:
  ```powershell
  Compress-Archive -Path Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3 -DestinationPath dist/AtmosViz_v<version>_Windows_VST3.zip -Force
  Compress-Archive -Path Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe -DestinationPath dist/AtmosViz_v<version>_Windows_Standalone.zip -Force
  Copy-Item build-clap/AtmosViz_artefacts/Release/AtmosViz.clap dist/AtmosViz_v<version>_Windows_CLAP.clap
  ```
- [ ] Smoke-test the zipped bundle and `.clap` in at least one host.

## Versioning
- [ ] Update version macros if required (`JuceLibraryCode/JucePluginDefines.h`).
- [ ] Commit changes with message `Release v<version>` (include doc updates and dist assets).
- [ ] Tag the commit: `git tag v<version>`.

## Publication
- [ ] Push commits and tags: `git push && git push --tags`.
- [ ] Create GitHub Release for `v<version>` and attach:
  - `dist/AtmosViz_v<version>_Windows_VST3.zip`
  - `dist/AtmosViz_v<version>_Windows_Standalone.zip`
  - `dist/AtmosViz_v<version>_Windows_CLAP.clap`
- [ ] Paste highlights from `docs/work_log.md` into release notes.

## Post-Release
- [ ] Open new issue(s) for outstanding TODO items.
- [ ] Archive fresh screenshots under `1/<yyyyMMdd>/` if visuals changed.
- [ ] Notify internal channels (`#atmosviz-dev`) with download link.
