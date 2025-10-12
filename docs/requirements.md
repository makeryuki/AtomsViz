# AtmosViz Requirements (2025-10-12)

## Functional Requirements
| ID | Description | Status |
|----|-------------|--------|
| FR-01 | Provide Inside and Outside room visualisations with selectable camera presets (Home, Front, Back, Left, Right, Top, User). | ✅ |
| FR-02 | Clamp Inside zoom to ≥40% (≥25% for Inside Top) and default to 60% (Top: 25%). | ✅ |
| FR-03 | Offer Radiation Heatmap view with density control (1–5) and colour legend. | ✅ |
| FR-04 | Expose Temporal Trails and Peak Hold visualisations with the same camera controls. | ✅ |
| FR-05 | Allow user adjustment of Low/Mid/High colour weights via sliders and Colour Mix Pad. | ✅ |
| FR-06 | Publish a ready-to-install VST3 bundle (Release build) as a distributable zip. | ✅ |
| FR-07 | Provide standalone executable mirroring plug-in functionality. | ✅ |
| FR-08 | Persist UI state (view mode, camera preset, zoom, colour weights) across sessions. | ✅ |
| FR-09 | Document build, usage, and release processes in the repository. | ✅ |

## Non-Functional Requirements
| ID | Description | Status |
|----|-------------|--------|
| NFR-01 | Maintain Outside view geometry as axis-aligned orthographic wireframe. | ✅ |
| NFR-02 | Ensure Inside view projects from the room origin without walking outside the bounds. | ✅ |
| NFR-03 | UI must remain legible at 1280×720 window size (header controls stay aligned and accessible). | ✅ |
| NFR-04 | Heatmap rendering updates within 16 ms/frame on reference hardware (Ryzen 7 3700X). | ⚠️ Manual check required |
| NFR-05 | Build automation script completes Release build in < 3 min on reference hardware. | ⚠️ Manifest helper warning pending |
| NFR-06 | Documentation stays in sync with codebase (review at each tagged release). | ✅ |

## Outstanding Items / Follow-Up
- Validate Radiation Heatmap coverage for all rows/columns (suspected sparse columns in user report).
- Adjust Inside Top orientation so the front wall maps to the bottom of the viewport.
- Consider adding automated visual regression (camera sweeps) to replace manual screenshot comparison.
- Decide whether to keep manifest generation disabled or migrate to native x64 helper.
