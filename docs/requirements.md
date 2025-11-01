# AtmosViz Requirements (2025-10-19)

## Functional Requirements
| ID | Description | Status |
|----|-------------|--------|
| FR-01 | Provide Inside and Outside room visualisations with selectable camera presets (Home, Front, Back, Left, Right, Top, User). | Done |
| FR-02 | Clamp Inside zoom to >=40% (>=25% for Inside Top) and default to 60% (Top: 25%). | Done |
| FR-03 | Offer Radiation Heatmap view with density control (levels 1-5) and colour legend. | Done |
| FR-04 | Expose Temporal Trails and Peak Hold visualisations with the same camera controls. | Done |
| FR-05 | Allow user adjustment of Low/Mid/High colour weights via sliders and Colour Mix Pad. | Done |
| FR-06 | Publish a ready-to-install VST3 bundle (Release build) as a distributable zip. | Done |
| FR-07 | Provide standalone executable mirroring plug-in functionality. | Done |
| FR-08 | Persist UI state (view mode, camera preset, zoom, colour weights) across sessions. | Done |
| FR-09 | Document build, usage, and release processes in the repository. | Done |
| FR-10 | Produce a CLAP plug-in using clap-juce-extensions and ship it alongside other artifacts. | Done |
| FR-11 | Publish macOS AU, VST3, Standalone, and CLAP bundles equivalent to Windows deliverables. | Done |
| FR-12 | Support Dolby Atmos 5.1 / 5.1.2 / 5.1.4 / 7.1.2 / 7.1.4 / 7.1.6 layouts (auto-detect via JUCE bus configuration). | Done |

## Non-Functional Requirements
| ID | Description | Status |
|----|-------------|--------|
| NFR-01 | Maintain Outside view geometry as axis-aligned orthographic wireframe. | Done |
| NFR-02 | Ensure Inside view projects from the room origin without walking outside the bounds. | Done |
| NFR-03 | UI must remain legible at 1280x720 window size (header controls stay aligned and accessible). | Done |
| NFR-04 | Heatmap rendering updates within 16 ms/frame on reference hardware (Ryzen 7 3700X). | Manual check required |
| NFR-05 | Build automation script completes Release build in < 3 min on reference hardware. | Manifest helper warning pending |
| NFR-06 | Documentation stays in sync with codebase (review at each tagged release). | Done |

## Outstanding Items / Follow-Up
- Validate Radiation Heatmap coverage for all rows/columns (suspected sparse columns in user report).
- Adjust Inside Top orientation so the front wall maps to the bottom of the viewport.
- Consider adding automated visual regression (camera sweeps) to replace manual screenshot comparison.
- Decide whether to keep manifest generation disabled or migrate to native x64 helper.
