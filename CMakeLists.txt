cmake_minimum_required(VERSION 3.21.1)

set(PROJECT_NAME NES_Emulator)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)
qt_standard_project_setup()

include_directories(include)

set(HEADERS
    include/core/apu.hpp
    include/core/bus.hpp
    include/core/cartridge.hpp
    include/core/controller.hpp
    include/core/cpu.hpp
    include/core/ppu.hpp
    include/core/mapper/mapper.hpp
    include/core/mapper/mapper0.hpp
    include/core/mapper/mapper1.hpp
    include/core/mapper/mapper2.hpp
    include/core/mapper/mapper3.hpp
    include/core/mapper/mapper4.hpp
    include/core/mapper/mapper7.hpp
    include/core/mapper/mapper9.hpp
    include/core/mapper/mapper66.hpp
    include/io/audioplayer.hpp
    include/io/emulatorthread.hpp
    include/io/iotypes.hpp
    include/io/mainwindow.hpp
    include/io/savestate.hpp
    include/io/threadsafeaudioqueue.hpp
    include/io/qtserializer.hpp
    include/util/circularbuffer.hpp
    include/util/serializer.hpp
    include/util/util.hpp
)

set(SOURCES
    src/core/apu.cpp
    src/core/bus.cpp 
    src/core/cartridge.cpp 
    src/core/controller.cpp 
    src/core/cpu.cpp
    src/core/ppu.cpp
    src/core/mapper/mapper.cpp
    src/core/mapper/mapper0.cpp
    src/core/mapper/mapper1.cpp
    src/core/mapper/mapper2.cpp
    src/core/mapper/mapper3.cpp
    src/core/mapper/mapper4.cpp
    src/core/mapper/mapper7.cpp
    src/core/mapper/mapper9.cpp
    src/core/mapper/mapper66.cpp
    src/io/audioplayer.cpp
    src/io/emulatorthread.cpp
    src/io/mainwindow.cpp
    src/io/savestate.cpp
    src/io/qtserializer.cpp
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
    Qt6::Multimedia
    Qt6::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
)