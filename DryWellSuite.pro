TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += drywell_legacy \
           model_creator_runner

# Keep the existing project untouched as the legacy/stable app target.
drywell_legacy.file = DryWellScriptGenerator.pro

# New executable where model creation + run + export workflows will be implemented.
model_creator_runner.subdir = apps/model_creator_runner
model_creator_runner.depends = drywell_legacy
