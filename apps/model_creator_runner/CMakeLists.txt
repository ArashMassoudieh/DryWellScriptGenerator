cmake_minimum_required(VERSION 3.16)

project(ModelCreatorRunner VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Test)

set(RUNNER_GENERATION_SOURCES
    src/structure_registry.cpp
    src/starter_script_builder.cpp
    src/structures/hq_drywell_builder.cpp
    src/structures/r_bioswale_builder.cpp
    src/structures/jm_bioretention_builder.cpp
    src/structures/vn_drywell_builder.cpp
)

add_library(model_creator_runner_core STATIC
    src/ohqdiscovery.cpp
    src/ohqprocessrunner.cpp
    ${RUNNER_GENERATION_SOURCES}
)
target_include_directories(model_creator_runner_core PUBLIC
    src
    src/structures
)
target_link_libraries(model_creator_runner_core PUBLIC Qt${QT_VERSION_MAJOR}::Core)

add_executable(ModelCreatorRunner
    src/main.cpp
    src/modelcreatorwindow.cpp
    src/simplelineplotwidget.cpp
    src/scripteditordialog.cpp
    resources/model_creator_runner.qrc
)
target_include_directories(ModelCreatorRunner PRIVATE src)
target_link_libraries(ModelCreatorRunner PRIVATE
    model_creator_runner_core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

include(CTest)
if(BUILD_TESTING)
    add_executable(model_creator_runner_tests
        tests/cpp/test_runner_core.cpp
    )
    target_link_libraries(model_creator_runner_tests PRIVATE
        model_creator_runner_core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME model_creator_runner_tests COMMAND model_creator_runner_tests)
endif()

install(TARGETS ModelCreatorRunner RUNTIME DESTINATION bin)
