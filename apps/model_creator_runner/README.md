# ModelCreatorRunner (work-in-progress)

This app is the new workflow target discussed for DryWellScriptGenerator evolution.

## Repository layout

- `src/app`: application entry point.
- `src/ui`: windows, dialogs, and plotting widgets.
- `src/execution`: OHQ process execution.
- `src/generation`: shared script-generation options and model registry.
- `src/generation/builders`: model-specific script builders.
- `resources/reference-models`: checked-in reference OHQ scripts.
- `tests`: automated analysis and generation checks.

The directories separate responsibilities without changing the runner's behavior.

## External OHQ discovery

The runner can discover OpenHydroQual from its saved UI settings and nearby
development directories. Standalone checkouts can also configure discovery with:

- `OHQ_EXECUTABLE` (or `OPENHYDROQUAL_EXECUTABLE`): OHQ executable path.
- `OHQ_ROOT` (or `OPENHYDROQUAL_ROOT`): OpenHydroQual installation/source root.
- `OHQ_TEMPLATE_DIR`: directory containing OHQ JSON template resources.

Explicit environment configuration takes precedence over automatic discovery.

## Planned flow
1. Create/select model input.
2. Generate `.ohq` script.
3. Run OHQ solver.
4. Save outputs, ERT-ready CSV snapshots, and VTK exports.

## Current state (updated)

- Minimal runner UI implemented with user-friendly options for **HQ_Drywell**, **VN_Drywell**, and **R_Bioswale** starter scripts.

- Starter generation now supports unified **build modes (presets)** across all structures:
  - `None`
  - `SoftReference`
  - `LoadFromOhq`
  - `FullReference`
  - (Generic `Preset` is no longer exposed in UI to avoid ambiguity)

---

## Build Mode Behavior

### General
- **FullReference**
  - Uses embedded canonical OHQ script.
  - Not affected by UI soil or geometry controls.

- **SoftReference**
  - Procedural generation mode.
  - Uses UI inputs to build blocks, links, and parameters.
  - Designed so default values reproduce FullReference behavior.

- **LoadFromOhq**
  - Uses user-provided `.ohq` as base and applies overrides.

- **None**
  - Minimal generation without enrichment logic.

---

## R_Bioswale (updated)

- **SoftReference now uses procedural Rosemead-style logic**, migrated from legacy `DialogRosemead`.

### Controlled via UI:
- bioswale width
- system/left width
- bioswale depth
- length
- lateral cells
- street width
- street cells
- anisotropy ratio
- soil properties file (optional)

### Behavior:
- Soil blocks are generated via loops:
  - `EngineeredSoil`
  - `LeftTop`
  - `RightTop`
  - `UEngineered`
  - `LeftBottom`
  - `RightBottom`
- Geometry is fully controlled by UI inputs.
- Soil parameters:
  - from CSV depth-profile if provided
  - otherwise fallback to embedded/reference-derived layers
- Engineered soil parameters are currently fixed unless extended.

---

## HQ_Drywell (updated)

- **SoftReference now supports soil parameter control**, aligned with legacy `DryWellDialog`.

### Soft soil parameter modes:
- `ReferenceDefaults`
  - Keeps original embedded HQ values (matches FullReference behavior)

- `Manual`
  - Uses UI-defined soft soil parameters

- `ModelCreatorDefaults`
  - Uses built-in defaults

- `File`
  - Uses depth-profile CSV and interpolates parameters by depth

### Notes:
- Current SoftReference:
  - updates **soil parameters**
  - preserves embedded geometry
- Geometry regeneration (nr, layers, well depth, etc.) is not yet migrated from legacy dialog.

---

## VN_Drywell

(Unchanged, already SoftReference-capable with grid controls)

---

## Important Notes

- UI values only affect output when using **SoftReference**.
- If output does not change:
  - ensure correct preset is selected
  - ensure project is rebuilt (no stale object files)
- FullReference is intentionally immutable.

---

## Architecture Direction

- Legacy dialogs (`DialogRosemead`, `DryWellDialog`) are being phased out.
- New architecture:

## JM_Bioretention (John McCormack Road)

A dedicated `JM_Bioretention` model type and procedural builder are included in
`jm_bioretention_builder.cpp/.h`. The checked-in reference output is
`apps/model_creator_runner/resources/reference-models/JM.ohq`.

Default SI geometry:
- facility: 12.192 m long × 3.7084 m wide
- mulch: 0.0762 m
- bioretention media: 0.9144 m
- choker: 0.0762 m
- gravel: 0.6096 m
- infiltration sump: 0.3048 m
- underdrain diameter: 0.1016 m

The generated `JM.ohq` uses four longitudinal cells matching the four CC-101
curb inlets, routes each surface/media/choker/gravel/sump layer vertically and
horizontally, connects each gravel cell to the 4 in underdrain, and routes the
last surface cell to the partial-height outlet at the local 0.0 m datum. The
reference grid is therefore `nx=4` by `nz=5` material layers, or 20 primary
hydraulic cells: 4 surface catchment cells, 4 media soil blocks, 4 choker
aggregate blocks, 4 gravel aggregate blocks, and 4 infiltration-sump/native-soil
blocks.

`FullReference` and default `SoftReference` use the same JM topology so the
checked-in reference, generated starter output, and UI preview stay comparable.
`SoftReference` still uses the JM geometry fields on `StarterScriptOptions`
(length, width, layer depths, underdrain diameter, and catchment area) to resize
that topology without changing the default cell counts.

JM follows the same high-level hydraulic pattern as `R_Bioswale`: one external
contributing catchment enters the surface-storage system, then connected
surface, engineered-media, aggregate-storage, native-soil, fixed-head, and
sewer/outlet links move water through the model. The main difference is that
JM uses a longitudinal four-cell profile for the CC-101 slope instead of the
Rosemead lateral street/bioswale grid.

The JM builder sources are registered in `model_creator_runner.pro` from
`src/generation/builders`.
