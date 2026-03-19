QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = ModelCreatorRunner

SOURCES += \
    main.cpp \
    modelcreatorwindow.cpp \
    ohqprocessrunner.cpp \
    simplelineplotwidget.cpp \
    starter_script_builder.cpp \
    scripteditordialog.cpp

HEADERS += \
    modelcreatorwindow.h \
    ohqprocessrunner.h \
    simplelineplotwidget.h \
    starter_script_builder.h \
    scripteditordialog.h
