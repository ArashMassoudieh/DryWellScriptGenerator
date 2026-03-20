# ModelCreatorRunner (work-in-progress)

This app is the new workflow target discussed for DryWellScriptGenerator evolution.

## Planned flow
1. Create/select model input.
2. Generate `.ohq` script.
3. Run OHQ solver.
4. Save outputs and VTK exports.

## Current state
- Minimal runner UI implemented with user-friendly options for both **Drywell** and **Bioswale** starter scripts.
- Starter generation inputs now include:
  - model type
  - template resources directory
  - generated script output path
  - model enrichment preset (optional blocks/links)
  - inflow file
  - simulation start/end
  - OHQ output series file name
  - optional observation file + soil layer/object + moisture expression + observation series name
  - optional raw "additional OHQ commands" appended to generated starter scripts
    - includes **Load file** to import reusable command snippets from `.txt`/`.ohq`
- Supports **Review/Edit .ohq**, **Generate starter .ohq**, and **Generate + Run** actions.
- Adds **Quick Run + Save** to apply suggested defaults, generate starter script, run OHQ, and save/copy artifacts to the artifacts directory in one flow.
- Built-in enrichment presets include monitoring-well, groundwater-boundary, pretreatment-chambers (drywell), and underdrain / underdrain+groundwater (bioswale) starter additions.
- Can execute an OHQ binary with a selected `.ohq` script and stream logs.
- Includes stop/cancel support for a running process.
- Includes an **Export run artifacts** action to copy discovered `.vtk/.vtp/.vtu/.csv/.txt` files from the working directory tree (preserving relative folders) and write `export_manifest.csv`.
- Persists last used paths/settings using `QSettings`.
- Detects newly modified output artifacts after each run and optionally copies them into a user-selected folder and writes `artifact_manifest.csv`.
- Lets you review and edit generated/draft scripts in a dedicated editor window before saving.
- Includes a **Plots** tab to visualize inflow inputs, output series, and observation data from numeric `.csv/.txt` files.
- Output plot now supports selecting **any numeric output parameter** for Y against any numeric X-axis column (e.g., radius/depth/time), enabling per-parameter inspection.
- Adds backend **depth-slice interpolation** (`Build depth slice`) to plot selected parameter-through-depth at a selected X/R location from output columns (or use an optional external depth-profile file).
- Adds **Export all depth slices** to dump through-depth profiles for all output parameters at the selected X/R into one CSV.
- Includes an **output-vs-observation comparison** summary (RMSE/MAE/Bias/R²) on the Plots tab, using X-axis interpolation for overlapping ranges to support ERT-style calibration checks.
- When an artifacts directory is configured, comparison runs append unique entries to `comparison_history.csv` for longitudinal calibration tracking (duplicate metrics snapshots are skipped).
- Includes **Clear history** to reset `comparison_history.csv` from the artifacts directory when starting a new calibration cycle.
- Adds **Export plot data** to save current output plot series, depth-slice profile, and comparison summary into a CSV plus a companion JSON analysis report (including structured comparison metrics when available).
- Includes a lightweight regression test script at `tests/test_analysis_algorithms.py` for interpolation/depth-slice math sanity checks.
