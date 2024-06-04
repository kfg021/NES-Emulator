QT = core gui widgets

HEADERS += src/core/bus.hpp src/core/cpu.hpp src/gui/mainwindow.hpp src/gui/gamewindow.hpp src/gui/debugwindow.hpp src/util/util.hpp
SOURCES += src/core/bus.cpp src/core/cpu.cpp src/gui/mainwindow.cpp src/gui/gamewindow.cpp src/gui/debugwindow.cpp src/util/util.cpp src/main.cpp

CONFIG += warn_off

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = moc