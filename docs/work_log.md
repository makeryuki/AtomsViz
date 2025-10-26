# AtmosViz Work Log

## 2025-10-27
- Added Visualization Gain control alongside existing frequency colour legend and rebalanced header layout.
- Removed legend captions for heatmap/frequency mapping to simplify UI while retaining colour cues.
- Built Windows Release artefacts (VST3, Standalone, CLAP) and prepared v0.4.0 packaging workflow.
- Updated clap CMake configuration to version 0.4.0.

## 2025-10-19
- Generated macOS Release builds (AU, VST3, Standalone) via Xcode and archived them under `dist/`.- Built macOS CLAP bundle with `/Applications/JUCE` toolchain and added macOS packaging commands.- Expanded README, developer guide, checklist, requirements, and user manual with macOS installation/build notes.
- Integrated clap-juce-extensions as a submodule and generated a Release CLAP build via CMake.
- Added standalone/CLAP distribution bundles under `dist/` and documented installation steps.
- Updated developer and release guides with CLAP build instructions.
- Added GPLv3 license and archived legacy documentation assets under `docs/_archive/20251012/`.

## 2025-10-12
- Normalised Inside zoom constraints (Home/User default 60%, Top default 25%, minimum 40%/25%).
- Adjusted Home/User orientation (yaw -95 deg, pitch -5 deg) to face front wall from origin.
- Rebuilt Release artefacts and published `dist/AtmosViz_v0.3.0_Windows_VST3.zip`.
- Refreshed documentation set (README, design docs, manuals) and archived legacy notes under `docs/_archive/20251012`.
- Verified Release standalone build launches successfully.
- Outstanding follow-up: Inside Top orientation tweak, Radiation Heatmap column coverage validation, evaluate manifest helper failure.

## 2025-10-11
- Completed matrix-based Inside projection rewrite to match `inside.html` reference.
- Restored Outside view stability after refactor.
- Tuned Colour Mix Pad call-out alignment and header layout (heatmap slider right-justified).

## 2025-10-07
- Added Band Colour Weights sliders and synchronised Colour Mix Pad interactions.
- Implemented heatmap density slider visibility toggle and legend updates for Radiation Heatmap mode.
- Created `build_vst3.ps1` automation script (Debug/Release support).

## 2025-09-30
- Introduced documentation restart notes and initial Zoom slider adjustments.
- Investigated VST3 manifest helper failure (`MSB3073`).

## 2025-09-20
- Initial design documentation drafted (superseded by current revision).


