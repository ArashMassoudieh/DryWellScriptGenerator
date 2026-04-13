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

SOURCES += \
    main.cpp \
    modelcreatorwindow.cpp \
    ohqprocessrunner.cpp \
    simplelineplotwidget.cpp \
    structure_registry.cpp \
    starter_script_builder.cpp \
    vn_drywell_builder.cpp \
    scripteditordialog.cpp

HEADERS += \
    modelcreatorwindow.h \
    ohqprocessrunner.h \
    simplelineplotwidget.h \
    structure_registry.h \
    starter_script_builder.h \
    vn_drywell_builder.h \
    scripteditordialog.h
