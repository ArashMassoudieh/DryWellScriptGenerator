# --------------------------------
# ModelCreatorRunner project
# - Lightweight runner UI for generating/running .ohq scripts.
# - Depends only on Qt modules listed below.
# - Does NOT compile importmoisturedata.* (that dialog remains in legacy app).
# --------------------------------
QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = ModelCreatorRunner

INCLUDEPATH += \
    src/ui \
    src/execution \
    src/generation \
    src/generation/builders

SOURCES += \
    src/app/main.cpp \
    src/ui/modelcreatorwindow.cpp \
    src/ui/simplelineplotwidget.cpp \
    src/ui/scripteditordialog.cpp \
    src/execution/ohqdiscovery.cpp \
    src/execution/ohqprocessrunner.cpp \
    src/generation/structure_registry.cpp \
    src/generation/starter_script_builder.cpp \
    src/generation/builders/hq_drywell_builder.cpp \
    src/generation/builders/r_bioswale_builder.cpp \
    src/generation/builders/jm_bioretention_builder.cpp \
    src/generation/builders/vn_drywell_builder.cpp

HEADERS += \
    src/ui/modelcreatorwindow.h \
    src/ui/simplelineplotwidget.h \
    src/ui/scripteditordialog.h \
    src/execution/ohqdiscovery.h \
    src/execution/ohqprocessrunner.h \
    src/generation/structure_registry.h \
    src/generation/starter_script_builder.h \
    src/generation/builders/hq_drywell_builder.h \
    src/generation/builders/r_bioswale_builder.h \
    src/generation/builders/jm_bioretention_builder.h \
    src/generation/builders/vn_drywell_builder.h

DISTFILES += \
    resources/reference-models/JM.ohq \
    resources/reference-models/JM_gutter.ohq
