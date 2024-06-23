QT = core gui widgets

# Uncomment the line below to compile a version that displays a side panel showing cpu status and pattern tables. May hurt performance.
# DEFINES += SHOW_DEBUG_WINDOW

HEADERS += \
src/core/bus.hpp \
src/core/cartridge.hpp \
src/core/controller.cpp \
src/core/cpu.hpp \
src/core/ppu.hpp \
src/core/mapper/mapper.hpp \
src/core/mapper/mapper0.hpp \
src/core/mapper/mapper2.hpp \
src/core/mapper/mapper3.hpp \
src/gui/debugwindow.hpp \
src/gui/gamewindow.hpp \
src/gui/mainwindow.hpp \
src/util/util.hpp

SOURCES += \
src/core/bus.cpp \
src/core/cartridge.cpp \
src/core/controller.cpp \
src/core/cpu.cpp \
src/core/ppu.cpp \
src/core/mapper/mapper.cpp \
src/core/mapper/mapper0.cpp \
src/core/mapper/mapper2.cpp \
src/core/mapper/mapper3.cpp \
src/gui/debugwindow.cpp \
src/gui/gamewindow.cpp \
src/gui/mainwindow.cpp \
src/util/util.cpp \
src/main.cpp

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
    OBJECTS_DIR = build/debug/.obj
    MOC_DIR = build/debug/.moc
}
CONFIG(release, debug|release) {
    DESTDIR = build/release
    OBJECTS_DIR = build/release/.obj
    MOC_DIR = build/release/.moc
}