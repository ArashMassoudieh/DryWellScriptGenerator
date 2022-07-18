QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
DEFINES += use_VTK
CONFIG += use_VTK
VTKBUILDPATH = /media/arash/E/Projects/VTK-9.1.0/VTK-build
VTKHEADERPATH = /media/arash/E/Projects/VTK-9.1.0
VTK_V = -9.1

# DEFINES += use_Armadillo
#CONFIG += Khiem
CONFIG += Arash
use_VTK {DEFINES += VTK}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../Utilities/BTC.cpp \
    ../Utilities/BTCSet.cpp \
    ../Utilities/DistributionNUnif.cpp \
    ../Utilities/Matrix.cpp \
    ../Utilities/NormalDist.cpp \
    ../Utilities/QuickSort.cpp \
    ../Utilities/Vector.cpp \
    ../Utilities/utilities.cpp \
    main.cpp \
    mainwindow.cpp \
    scad_generator.cpp \
    vtkdialog.cpp

HEADERS += \
    ../Utilities/BTC.h \
    ../Utilities/BTCSet.h \
    ../Utilities/DistributionNUnif.h \
    ../Utilities/Matrix.h \
    ../Utilities/NormalDist.h \
    ../Utilities/QuickSort.h \
    ../Utilities/Vector.h \
    ../Utilities/utilities.h \
    VTK.h \
    mainwindow.h \
    scad_generator.h \
    vtkdialog.h
Khiem {
    message("Khiem's version")
}

Arash {
    message("Arash's version")
}

use_VTK {
    message("using VTK")
}

FORMS += \
    mainwindow.ui \
    vtkdialog.ui

TRANSLATIONS += \
    DryWellScriptGenerator_en_US.ts

INCLUDEPATH += ../Utilities/
Khiem {
    use_VTK {
        INCLUDEPATH += D:\Software\VTK-8.2.0\include\vtk-8.2

        LIBS+=  D:\Software\VTK-8.2.0\lib\vtkChartsCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonColor-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonComputationalGeometry-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonDataModel-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonExecutionModel-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonMath-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonMisc-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonSystem-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkCommonTransforms-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkDICOMParser-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkDomainsChemistry-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkdoubleconversion-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkexodusII-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkexpat-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersAMR-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersExtraction-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersFlowPaths-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersGeneral-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersGeneric-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersGeometry-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersHybrid-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersHyperTree-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersImaging-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersModeling-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersParallel-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersParallelImaging-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersPoints-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersProgrammable-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersSelection-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersSMP-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersSources-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersStatistics-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersTexture-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersTopology-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkFiltersVerdict-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkfreetype-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkGeovisCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkgl2ps-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkglew-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkhdf5-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkhdf5_hl-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingColor-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingFourier-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingGeneral-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingHybrid-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingMath-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingMorphological-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingSources-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingStatistics-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkImagingStencil-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkInfovisCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkInfovisLayout-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkInteractionImage-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkInteractionStyle-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkInteractionWidgets-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOAMR-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOAsynchronous-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOCityGML-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOEnSight-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOExodus-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOExport-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOExportPDF-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOGeometry-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOImage-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOImport-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOInfovis-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOLegacy-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOLSDyna-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOMINC-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOMovie-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIONetCDF-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOParallel-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOParallelXML-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOPLY-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOSegY-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOSQL-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOTecplotTable-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOVeraOut-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOVideo-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOXML-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkIOXMLParser-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkjpeg-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkjsoncpp-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtklibharu-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtklibxml2-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtklz4-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtklzma-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkmetaio-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtknetcdf-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkogg-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkParallelCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkpng-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkpugixml-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingAnnotation-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingContext2D-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingFreeType-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingGL2PSOpenGL2-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingImage-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingLabel-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingLOD-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingOpenGL2-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingVolume-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkRenderingVolumeOpenGL2-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtksqlite-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtksys-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtktheora-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtktiff-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkverdict-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkViewsContext2D-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkViewsCore-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkViewsInfovis-8.2.lib \
                D:\Software\VTK-8.2.0\lib\vtkzlib-8.2.lib
        }
}
Arash {

    use_VTK {
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkChartsCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonColor$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonComputationalGeometry$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonDataModel$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonExecutionModel$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonMath$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonMisc$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonSystem$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkCommonTransforms$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkDICOMParser$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkexpat$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersAMR$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersExtraction$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersFlowPaths$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersGeneral$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersGeneric$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersGeometry$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersHybrid$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersHyperTree$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersImaging$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersModeling$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersParallel$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersParallelImaging$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersPoints$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersProgrammable$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersSelection$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersSMP$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersSources$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersStatistics$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersTexture$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersTopology$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkFiltersVerdict$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkfreetype$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkGeovisCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkgl2ps$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkglew$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkhdf5$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkhdf5_hl$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingColor$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingFourier$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingGeneral$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingHybrid$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingMath$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingMorphological$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingSources$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingStatistics$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkImagingStencil$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkInfovisCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkInfovisLayout$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkInteractionImage$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkInteractionStyle$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkInteractionWidgets$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOAMR$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOEnSight$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOExodus$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOGeometry$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOImage$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOImport$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOInfovis$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOLegacy$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOLSDyna$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOMINC$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOMovie$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIONetCDF$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOParallel$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOParallelXML$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOPLY$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOSQL$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOTecplotTable$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOVideo$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOXML$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkIOXMLParser$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkjpeg$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkjsoncpp$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtklibharu$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtklibxml2$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtklz4$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkmetaio$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkParallelCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkpng$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingAnnotation$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingContext2D$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingFreeType$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingGL2PSOpenGL2$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingImage$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingLabel$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingLOD$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingOpenGL2$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingVolume$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkRenderingVolumeOpenGL2$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtksqlite$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtksys$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtktiff$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkverdict$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkViewsContext2D$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkViewsCore$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkViewsInfovis$$VTK_V
    LIBS += -L$$VTKBUILDPATH/lib/ -lvtkzlib$$VTK_V
    LIBS += -L"/usr/local/lib/ -lsuperlu.so"

    #VTK Include files
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/Core
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/Color
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/Color
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/DataModel
    INCLUDEPATH +=$${VTKBUILDPATH}/Utilities/KWIML
    INCLUDEPATH +=$${VTKHEADERPATH}/Utilities/KWIML
    INCLUDEPATH +=$${VTKHEADERPATH}/Rendering/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Rendering/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Filters/Core
    INCLUDEPATH +=$${VTKHEADERPATH}/Charts/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Charts/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Filters/General
    INCLUDEPATH +=$${VTKBUILDPATH}/Rendering/Context2D
    INCLUDEPATH +=$${VTKHEADERPATH}/Rendering/Context2D
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/DataModel
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/Math
    INCLUDEPATH +=$${VTKHEADERPATH}/Views/Context2D
    INCLUDEPATH +=$${VTKBUILDPATH}/Views/Context2D
    INCLUDEPATH +=$${VTKBUILDPATH}/Views/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Interaction/Widgets
    INCLUDEPATH +=$${VTKHEADERPATH}/Views/Core
    INCLUDEPATH +=$${VTKHEADERPATH}/Interaction/Style
    INCLUDEPATH +=$${VTKBUILDPATH}/Interaction/Style
    INCLUDEPATH +=$${VTKHEADERPATH}/Filters/Modeling
    INCLUDEPATH +=$${VTKBUILDPATH}/Filters/Modeling
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/ExecutionModel
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/ExecutionModel
    INCLUDEPATH +=$${VTKHEADERPATH}/Interaction/Widgets/
    INCLUDEPATH +=$${VTKHEADERPATH}/Filters/Core/
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/Misc/
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/Misc
    INCLUDEPATH +=$${VTKHEADERPATH}/IO/XML/
    INCLUDEPATH +=$${VTKBUILDPATH}/IO/XML
    INCLUDEPATH +=$${VTKHEADERPATH}/Filters/Sources
    INCLUDEPATH +=$${VTKBUILDPATH}/Filters/Sources
    INCLUDEPATH +=$${VTKHEADERPATH}/Filters/General
    INCLUDEPATH +=$${VTKHEADERPATH}/IO/Image
    INCLUDEPATH +=$${VTKBUILDPATH}/IO/Image
    INCLUDEPATH +=$${VTKHEADERPATH}/Imaging/Core
    INCLUDEPATH +=$${VTKBUILDPATH}/Imaging/Core

    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


