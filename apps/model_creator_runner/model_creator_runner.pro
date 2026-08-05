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
    src \
    src/structures

SOURCES += \
    src/main.cpp \
    src/modelcreatorwindow.cpp \
    src/simplelineplotwidget.cpp \
    src/scripteditordialog.cpp \
    src/ohqdiscovery.cpp \
    src/ohqprocessrunner.cpp \
    src/structure_registry.cpp \
    src/starter_script_builder.cpp \
    src/structures/hq_drywell_builder.cpp \
    src/structures/r_bioswale_builder.cpp \
    src/structures/jm_bioretention_builder.cpp \
    src/structures/vn_drywell_builder.cpp

HEADERS += \
    src/modelcreatorwindow.h \
    src/simplelineplotwidget.h \
    src/scripteditordialog.h \
    src/ohqdiscovery.h \
    src/ohqprocessrunner.h \
    src/structure_registry.h \
    src/starter_script_builder.h \
    src/structures/hq_drywell_builder.h \
    src/structures/r_bioswale_builder.h \
    src/structures/jm_bioretention_builder.h \
    src/structures/vn_drywell_builder.h

DISTFILES += \
    CMakeLists.txt \
    resources/reference-models/JM.ohq \
    resources/reference-models/JM_gutter.ohq \
    docs/EXTRACTION.md

RESOURCES += resources/model_creator_runner.qrc
