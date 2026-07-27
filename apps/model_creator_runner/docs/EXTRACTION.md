# Standalone repository extraction

The runner is built as an independent application and does not compile the
legacy DryWellScriptGenerator UI. Before extracting, require the CMake build,
C++ tests, and Python resource checks to pass in CI.

From a disposable clone, preserve the runner history with:

```bash
git filter-repo \
  --path apps/model_creator_runner/ \
  --path-rename apps/model_creator_runner/:
```

Then add the new remote and push the filtered branch. Do not run this command in
the primary working clone. After the new repository is verified, replace the
old directory with a migration notice rather than a submodule unless the legacy
application must pin and build the runner.
