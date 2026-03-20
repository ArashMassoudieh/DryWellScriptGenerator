#####################################################################
# Qt Version Auto-Config (Qt5 / Qt6)
#####################################################################

QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 5) {
    message(">>> Building with Qt 6.x")
    QT += core5compat
    DEFINES += QT6_BUILD
} else {
    message(">>> Building with Qt 5.x")
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtWidgets
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtGui
    DEFINES += QT5_BUILD
}

#####################################################################
# C++ Standard
#####################################################################
CONFIG -= c++11
CONFIG += c++17
CONFIG += no_lflags_merge
QMAKE_CXXFLAGS += -std=c++17

#####################################################################
# OpenMP
#####################################################################
unix:!macx {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS   += -fopenmp
    QMAKE_LFLAGS   += -Wl,--no-as-needed
    LIBS           += -lgomp
}

#####################################################################
# Armadillo / BLAS / LAPACK (Correct Order!)
#####################################################################
unix:!macx {
    LIBS += -larmadillo
    LIBS += -lopenblas -llapack -lblas
    LIBS += -lgfortran -lpthread -lm
}

#####################################################################
# GSL
#####################################################################
DEFINES += GSL
LIBS += -lgsl -lgslcblas -lm

#####################################################################
# Build Profile
#####################################################################
CONFIG += use_VTK
DEFINES += use_VTK

#####################################################################
# Machine-Specific OHQPATH
#####################################################################

CONFIG += PowerEdge
DEFINES += PowerEdge

#CONFIG += Hooman
#CONFIG += Arash
#CONFIG += SligoCreek

CONFIG(PowerEdge) {
    OHQPATH = /mnt/3rd900/Projects/OpenHydroQual/aquifolium
    VTKHEADERPATH = /mnt/3rd900/Projects/VTK
    VTKBUILDPATH  = /mnt/3rd900/Projects/VTK-build
    VTK_V = -9.0
}

CONFIG(Hooman) {
    OHQPATH = /home/hoomanmoradpour/Projects/OpenHydroQual/aquifolium
    VTKHEADERPATH = /home/hoomanmoradpour/Projects/VTK-9.3.0
    VTKBUILDPATH  = /home/hoomanmoradpour/Projects/VTK-9.3.0/VTK-build
    VTK_V = -9.3
}

CONFIG(Arash) {
    OHQPATH = /home/arash/Projects/OpenHydroQual/aquifolium
    VTKHEADERPATH = /home/arash/Projects/VTK
    VTKBUILDPATH  = /home/arash/Projects/VTK/VTK-build
    VTK_V = -9.2
}

CONFIG(SligoCreek) {
    OHQPATH = /media/arash/E/Projects/OpenHydroQual/aquifolium
    VTKHEADERPATH = /media/arash/E/Projects/VTK-9.1.0
    VTKBUILDPATH  = /media/arash/E/Projects/VTK-9.1.0/VTK-build
    VTK_V = -9.1
}

#####################################################################
# Include Paths
#####################################################################

INCLUDEPATH += $$OHQPATH
INCLUDEPATH += $$OHQPATH/include
INCLUDEPATH += $$OHQPATH/include/GA
INCLUDEPATH += $$OHQPATH/include/MCMC
INCLUDEPATH += $$OHQPATH/src

INCLUDEPATH += $$OHQPATH/../jsoncpp/include
INCLUDEPATH += ../Utilities

#####################################################################
# Project Sources / Headers / Forms
#####################################################################

SOURCES += \
    DryWellDialog.cpp \
    dialogrosemead.cpp \
    importmoisturedata.cpp \
    main.cpp \
    mainwindow.cpp \
    paths.cpp \
    postprocess.cpp \
    qt_jsonvalue_compat.cpp \
    scad_generator.cpp \
    solver_runner.cpp \
    threedmap.cpp \
    ../Utilities/cpoint.cpp \
    ../Utilities/cpoint3d.cpp

HEADERS += \
    DryWellDialog.h \
    dialogrosemead.h \
    importmoisturedata.h \
    mainwindow.h \
    paths.h \
    postprocess.h \
    scad_generator.h \
    solver_runner.h \
    threedmap.h \
    ../Utilities/cpoint.h \
    ../Utilities/cpoint3d.h \
    ../Utilities/cpointset.hpp

FORMS += \
    DryWellDialog.ui \
    dialogrosemead.ui \
    importmoisturedata.ui \
    mainwindow.ui \
    postprocess.ui

TRANSLATIONS += DryWellScriptGenerator_en_US.ts

#####################################################################
# VTK CONFIGURATION
#####################################################################

CONFIG(use_VTK) {
    HEADERS += VTK.h vtkdialog.h
    SOURCES += vtkdialog.cpp
    FORMS   += vtkdialog.ui

    INCLUDEPATH += $${VTKHEADERPATH}
    INCLUDEPATH += $${VTKBUILDPATH}

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
    INCLUDEPATH += $${VTKHEADERPATH}/Common/ExecutionModel
    INCLUDEPATH += $${VTKBUILDPATH}/Common/ExecutionModel
    INCLUDEPATH += $${VTKHEADERPATH}/Common/System
    INCLUDEPATH += $${VTKBUILDPATH}/Common/System

    INCLUDEPATH += $${VTKHEADERPATH}/Utilities/KWIML
    INCLUDEPATH += $${VTKBUILDPATH}/Utilities/KWIML
    INCLUDEPATH += $${VTKBUILDPATH}/Utilities/KWSys

    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Core
    INCLUDEPATH += $${VTKHEADERPATH}/Filters/General
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/General
    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Geometry
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Geometry
    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Sources
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Sources
    INCLUDEPATH += $${VTKHEADERPATH}/Filters/Hybrid
    INCLUDEPATH += $${VTKBUILDPATH}/Filters/Hybrid

    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/Core
    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/OpenGL2
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/OpenGL2
    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/FreeType
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/FreeType
    INCLUDEPATH += $${VTKHEADERPATH}/Rendering/Context2D
    INCLUDEPATH += $${VTKBUILDPATH}/Rendering/Context2D

    INCLUDEPATH += $${VTKHEADERPATH}/Views/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Views/Core
    INCLUDEPATH += $${VTKHEADERPATH}/Views/Context2D
    INCLUDEPATH += $${VTKBUILDPATH}/Views/Context2D

    INCLUDEPATH += $${VTKHEADERPATH}/Charts/Core
    INCLUDEPATH += $${VTKBUILDPATH}/Charts/Core

    INCLUDEPATH += $${VTKHEADERPATH}/IO/XML
    INCLUDEPATH += $${VTKBUILDPATH}/IO/XML
    INCLUDEPATH += $${VTKHEADERPATH}/IO/XMLParser
    INCLUDEPATH += $${VTKBUILDPATH}/IO/XMLParser
    INCLUDEPATH += $${VTKHEADERPATH}/IO/Core
    INCLUDEPATH += $${VTKBUILDPATH}/IO/Core
    INCLUDEPATH += $${VTKHEADERPATH}/IO/Legacy
    INCLUDEPATH += $${VTKBUILDPATH}/IO/Legacy

    LIBS += -L$${VTKBUILDPATH}/lib \
        -lvtkCommonCore$${VTK_V} \
        -lvtkCommonExecutionModel$${VTK_V} \
        -lvtkCommonDataModel$${VTK_V} \
        -lvtkCommonMath$${VTK_V} \
        -lvtkCommonTransforms$${VTK_V} \
        -lvtkCommonMisc$${VTK_V} \
        -lvtkFiltersCore$${VTK_V} \
        -lvtkFiltersGeneral$${VTK_V} \
        -lvtkFiltersGeometry$${VTK_V} \
        -lvtkFiltersHybrid$${VTK_V} \
        -lvtkFiltersSources$${VTK_V} \
        -lvtkImagingCore$${VTK_V} \
        -lvtkRenderingCore$${VTK_V} \
        -lvtkRenderingOpenGL2$${VTK_V} \
        -lvtkRenderingFreeType$${VTK_V} \
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
# Link OpenHydroQual static library LAST
#####################################################################

OPENHYDROQUAL_STATIC = $${PWD}/OHQ-static
LIBS += -L$${OPENHYDROQUAL_STATIC} -lOpenHydroQual

# Keep QtCore after OpenHydroQual for static-lib dependent symbol resolution
# (e.g., QJsonValueConstRef symbols referenced from libOpenHydroQual.a).
greaterThan(QT_MAJOR_VERSION, 5) {
    LIBS += -L$$[QT_INSTALL_LIBS] -lQt6Core -lQt6Core5Compat
} else {
    LIBS += -L$$[QT_INSTALL_LIBS] -lQt5Core
}

#####################################################################
# Misc
#####################################################################
DISTFILES += ~AutoRecover.DryWellScriptGenerator.vcxproj
