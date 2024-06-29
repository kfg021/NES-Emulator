# mkdir build/ && cd build/
# cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo ..
# make

cmake_minimum_required(VERSION 3.21.1)

set(PROJECT_NAME NES_Emulator)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

include_directories(include)

set(HEADERS
    include/core/bus.hpp
    include/core/cartridge.hpp
    include/core/controller.hpp
    include/core/cpu.hpp
    include/core/ppu.hpp
    include/core/mapper/mapper.hpp
    include/core/mapper/mapper0.hpp
    include/core/mapper/mapper2.hpp
    include/core/mapper/mapper3.hpp
    include/core/mapper/mapper66.hpp
    include/gui/debugwindow.hpp
    include/gui/gamewindow.hpp
    include/gui/mainwindow.hpp
    include/util/util.hpp
)

set(SOURCES
    src/core/bus.cpp 
    src/core/cartridge.cpp 
    src/core/controller.cpp 
    src/core/cpu.cpp
    src/core/ppu.cpp
    src/core/mapper/mapper.cpp
    src/core/mapper/mapper0.cpp
    src/core/mapper/mapper2.cpp
    src/core/mapper/mapper3.cpp
    src/core/mapper/mapper66.cpp
    src/gui/debugwindow.cpp
    src/gui/gamewindow.cpp
    src/gui/mainwindow.cpp
    src/util/util.cpp
    src/main.cpp
)

qt_add_executable(${PROJECT_NAME} 
    ${HEADERS} 
    ${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE ON
    WIN32_EXECUTABLE ON
    OUTPUT_NAME ${PROJECT_NAME}
)

# Uncomment the line below to compile a version that displays a side panel showing cpu status and pattern tables. May hurt performance.
# target_compile_definitions(${PROJECT_NAME} PRIVATE SHOW_DEBUG_WINDOW)