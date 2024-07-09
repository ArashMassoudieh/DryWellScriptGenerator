
QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
DEFINES += use_VTK
CONFIG += use_VTK

# DEFINES += use_Armadillo
#CONFIG += Khiem
CONFIG += Hooman
DEFINES += Hooman
#DEFINES += ModelCatchments
#CONFIG += Brett
#DEFINES += Brett
use_VTK {DEFINES += VTK}


#VTKBUILDPATH = /home/arash/Projects/VTK-9.2.0/
Arash
{
    VTKBUILDPATH = /home/arash/Projects/VTK/VTK-build
    VTKHEADERPATH = /home/arash/Projects/VTK
}
Hooman
{
    VTKBUILDPATH = /home/hoomanmoradpour/Projects/VTK-9.3.0/VTK-build
    VTKHEADERPATH = /home/hoomanmoradpour/Projects/VTK-9.3.0
}
#Arash Home PowerEdge
#VTKBUILDPATH = /mnt/3rd900/Projects/VTK-build
#VTKHEADERPATH = /mnt/3rd900/Projects/VTK

VTK_V = -9.2
Hooman
{
    VTK_V = -9.3
}



# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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

Khiem {
    message("Khiem's version")
}

Arash {
    message("Arash's version")
}

Hooman {
    message("Hooman's version")
}

use_VTK {
    message("using VTK")
    HEADERS += VTK.h
    FORMS += vtkdialog.ui
    HEADERS += vtkdialog.h
    SOURCES += vtkdialog.cpp
}

FORMS += \
    DryWellDialog.ui \
    dialogrosemead.ui \
    importmoisturedata.ui \
    mainwindow.ui \
    postprocess.ui


TRANSLATIONS += \
    DryWellScriptGenerator_en_US.ts

INCLUDEPATH += ../Utilities/
Brett {
    use_VTK {
        INCLUDEPATH += ..\VTK-9.2.0\include\vtk-9.2

        LIBS+=  -L..\VTK-9.2.0\lib\ -lvtkChartsCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonColor-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonComputationalGeometry-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonDataModel-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonExecutionModel-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonMath-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonMisc-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonSystem-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkCommonTransforms-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkDICOMParser-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkDomainsChemistry-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkexpat-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersAMR-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersExtraction-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersFlowPaths-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersGeneral-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersGeneric-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersGeometry-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersHybrid-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersHyperTree-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersImaging-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersModeling-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersParallel-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersParallelImaging-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersPoints-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersProgrammable-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersSelection-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersSMP-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersSources-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersStatistics-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersTexture-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersTopology-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkFiltersVerdict-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkfreetype-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkGeovisCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkgl2ps-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkglew-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkhdf5-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkhdf5_hl-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingColor-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingFourier-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingGeneral-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingHybrid-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingMath-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingMorphological-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingSources-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingStatistics-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkImagingStencil-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkInfovisCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkInfovisLayout-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkInteractionImage-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkInteractionStyle-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkInteractionWidgets-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOAMR-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOEnSight-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOExodus-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOExport-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOGeometry-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOImage-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOImport-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOInfovis-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOLegacy-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOLSDyna-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOMINC-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOMovie-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIONetCDF-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOParallel-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOParallelXML-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOPLY-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOSQL-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOTecplotTable-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOVideo-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOXML-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkIOXMLParser-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkjpeg-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkjsoncpp-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtklibharu-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtklibxml2-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtklz4-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkmetaio-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtknetcdf-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkParallelCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkpng-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingAnnotation-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingContext2D-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingFreeType-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingGL2PSOpenGL2-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingImage-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingLabel-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingLOD-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingOpenGL2-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingVolume-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkRenderingVolumeOpenGL2-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtksqlite-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtksys-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtktiff-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkverdict-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkViewsContext2D-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkViewsCore-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkViewsInfovis-9.2 \
                -L..\VTK-9.2.0\lib\ -lvtkzlib-9.2
        }
}

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
    INCLUDEPATH +=$${VTKBUILDPATH}//Utilities/KWSys
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/Core
    INCLUDEPATH +=$${VTKHEADERPATH}/Common/Transforms
    INCLUDEPATH +=$${VTKBUILDPATH}/Common/Transforms
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

DISTFILES += \
    ~AutoRecover.DryWellScriptGenerator.vcxproj

