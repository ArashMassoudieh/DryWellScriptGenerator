# --------------------------------
# DryWellSuite meta-project
# - Builds subprojects in a fixed order.
# - Keeps legacy app as baseline target and builds runner alongside it.
# --------------------------------
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += drywell_legacy \
           model_creator_runner

# Keep the existing project untouched as the legacy/stable app target.
# This points to the original DryWell desktop application.
# NOTE: importmoisturedata.cpp/.h/.ui live in this legacy target.
drywell_legacy.file = DryWellScriptGenerator.pro

# New executable where model creation + run + export workflows will be implemented.
# Runner build depends on legacy target so shared assumptions remain valid.
model_creator_runner.subdir = apps/model_creator_runner
model_creator_runner.depends = drywell_legacy
