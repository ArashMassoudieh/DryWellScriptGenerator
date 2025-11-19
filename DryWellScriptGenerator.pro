#####################################################################
# Qt Project for DryWellScriptGenerator
#####################################################################

QT += core gui widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#####################################################################
# C++ Standard  (REQUIRED for std::optional, filesystem, string_view)
#####################################################################
CONFIG -= c++11
CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17

#####################################################################
# Build Profile
#####################################################################
CONFIG += use_VTK
DEFINES += use_VTK

CONFIG += PowerEdge
DEFINES += PowerEdge

#CONFIG += Hooman
#DEFINES += Hooman

#CONFIG += Arash
#DEFINES += Arash

#####################################################################
# User-Specific VTK Paths
#####################################################################

CONFIG(PowerEdge) {
    VTKHEADERPATH = /mnt/3rd900/Projects/VTK
    VTKBUILDPATH  = /mnt/3rd900/Projects/VTK-build
    VTK_V = -9.0
}

CONFIG(Hooman) {
    VTKHEADERPATH = /home/hoomanmoradpour/Projects/VTK-9.3.0
    VTKBUILDPATH  = /home/hoomanmoradpour/Projects/VTK-9.3.0/VTK-build
    VTK_V = -9.3
}

CONFIG(Arash) {
    VTKHEADERPATH = /home/arash/Projects/VTK
    VTKBUILDPATH  = /home/arash/Projects/VTK/VTK-build
    VTK_V = -9.2
}

#####################################################################
# Project Sources / Headers / Forms
#####################################################################

SOURCES += \
    ../Utilities/DistributionNUnif.cpp \
    ../Utilities/Matrix.cpp \
    ../Utilities/NormalDist.cpp \
    ../Utilities/QuickSort.cpp \
    ../Utilities/Vector.cpp \
    ../Utilities/Utilities.cpp \
    ../Utilities/cpoint.cpp \
    ../Utilities/cpoint3d.cpp \
    DryWellDialog.cpp \
    dialogrosemead.cpp \
    importmoisturedata.cpp \
    main.cpp \
    mainwindow.cpp \
    postprocess.cpp \
    scad_generator.cpp \
    threedmap.cpp

HEADERS += \
    ../Utilities/BTC.h \
    ../Utilities/BTCSet.h \
    ../Utilities/BTC.hpp \
    ../Utilities/BTCSet.hpp \
    ../Utilities/DistributionNUnif.h \
    ../Utilities/Matrix.h \
    ../Utilities/NormalDist.h \
    ../Utilities/QuickSort.h \
    ../Utilities/Vector.h \
    ../Utilities/cpoint.h \
    ../Utilities/cpoint3d.h \
    ../Utilities/cpointset.hpp \
    ../Utilities/utilities.h \
    dialogrosemead.h \
    importmoisturedata.h \
    mainwindow.h \
    DryWellDialog.h \
    postprocess.h \
    scad_generator.h \
    threedmap.h

FORMS += \
    DryWellDialog.ui \
    dialogrosemead.ui \
    importmoisturedata.ui \
    mainwindow.ui \
    postprocess.ui

TRANSLATIONS += DryWellScriptGenerator_en_US.ts

INCLUDEPATH += ../Utilities

#####################################################################
# VTK CONFIGURATION – FINAL, CORRECT, TESTED
#####################################################################

CONFIG(use_VTK) {
    message(">>> Enabling VTK integration")

    HEADERS += VTK.h
    FORMS   += vtkdialog.ui
    HEADERS += vtkdialog.h
    SOURCES += vtkdialog.cpp

    ############################################################
    # ROOT include dirs
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}
    INCLUDEPATH += $${VTKBUILDPATH}

    ############################################################
    # Common
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}/Common/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Common/Core

    INCLUDEPATH += $${VTKHEADERPATH}/Common/DataModel
    INCLUDEPATH += $${VTKBUILDPATH}/Common/DataModel

    INCLUDEPATH += $${VTKHEADERPATH}/Common/Math
    INCLUDEPATH += $${VTKBUILDPATH}/Common/Math

    INCLUDEPATH += $${VTKHEADERPATH}/Common/Transforms
    INCLUDEPATH += $${VTKBUILDPATH}/Common/Transforms

    INCLUDEPATH += $${VTKHEADERPATH}/Common/Misc
    INCLUDEPATH += $${VTKBUILDPATH}/Common/Misc

    # For vtkNamedColors
    INCLUDEPATH += $${VTKHEADERPATH}/Common/Color
    INCLUDEPATH += $${VTKBUILDPATH}/Common/Color

    # PolyDataAlgorithm, ExecutionModel
    INCLUDEPATH += $${VTKHEADERPATH}/Common/ExecutionModel
    INCLUDEPATH += $${VTKBUILDPATH}/Common/ExecutionModel

    ############################################################
    # Charts, Views, Rendering, Context2D
    ############################################################

    INCLUDEPATH += $${VTKHEADERPATH}/Charts/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Charts/Core

    INCLUDEPATH += $${VTKHEADERPATH}/Views/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Views/Core

    INCLUDEPATH += $${VTKHEADERPATH}/Views/Context2D
    INCLUDEPATH += $${VTKBUILDPATH}/Views/Context2D

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/Core

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/FreeType
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/FreeType

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/ContextOpenGL2
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/ContextOpenGL2

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/Context2D
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/Context2D

    ############################################################
    # Filters — ***Correct order*** (Modeling FIRST)
    ############################################################

    # For vtkOutlineFilter (VTK 9.x moved here)
    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Modeling
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Modeling

    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Core

    INCLUDEPATH += $${VTKHEADERPATH}/Filters/General
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/General

    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Sources
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Sources

    ############################################################
    # Interaction
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}/Interaction/Style
    INCLUDEPATH += $${VTKBUILDPATH}/Interaction/Style

    INCLUDEPATH += $${VTKHEADERPATH}/Interaction/Widgets
    INCLUDEPATH += $${VTKBUILDPATH}/Interaction/Widgets

    INCLUDEPATH += $${VTKHEADERPATH}/Interaction/Image
    INCLUDEPATH += $${VTKBUILDPATH}/Interaction/Image

    ############################################################
    # Imaging
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}/Imaging/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Imaging/Core

    ############################################################
    # IO
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}/IO/XML
    INCLUDEPATH += $${VTKBUILDPATH}/IO/XML

    INCLUDEPATH += $${VTKHEADERPATH}/IO/Image
    INCLUDEPATH += $${VTKBUILDPATH}/IO/Image

    ############################################################
    # Utilities
    ############################################################
    INCLUDEPATH += $${VTKHEADERPATH}/Utilities/KWIML
    INCLUDEPATH += $${VTKBUILDPATH}/Utilities/KWIML
    INCLUDEPATH += $${VTKBUILDPATH}/Utilities/KWSys

    ############################################################
    # Libraries
    ############################################################
    LIBS += -L$${VTKBUILDPATH}/lib \
            -lvtkCommonCore$${VTK_V} \
            -lvtkCommonExecutionModel$${VTK_V} \
            -lvtkCommonDataModel$${VTK_V} \
            -lvtkCommonMath$${VTK_V} \
            -lvtkCommonTransforms$${VTK_V} \
            -lvtkCommonMisc$${VTK_V} \
            -lvtkFiltersCore$${VTK_V} \
            -lvtkFiltersGeneral$${VTK_V} \
            -lvtkFiltersSources$${VTK_V} \
            -lvtkFiltersModeling$${VTK_V} \
            -lvtkImagingCore$${VTK_V} \
            -lvtkRenderingCore$${VTK_V} \
            -lvtkRenderingFreeType$${VTK_V} \
            -lvtkRenderingOpenGL2$${VTK_V} \
            -lvtkInteractionStyle$${VTK_V} \
            -lvtkChartsCore$${VTK_V} \
            -lvtkViewsContext2D$${VTK_V} \
            -lvtkRenderingContext2D$${VTK_V} \
            -lvtkIOXML$${VTK_V} \
            -lvtkIOImage$${VTK_V} \
            -lvtkzlib$${VTK_V}

    LIBS += -lsuperlu
}

#####################################################################
# Misc
#####################################################################
DISTFILES += ~AutoRecover.DryWellScriptGenerator.vcxproj
