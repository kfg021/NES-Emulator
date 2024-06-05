QT = core gui widgets

HEADERS += \
src/core/bus.hpp \
src/core/cpu.hpp \
src/core/cartridge.hpp \
src/core/mapper/mapper.hpp \
src/core/mapper/mapper0.hpp \
src/gui/mainwindow.hpp \
src/gui/gamewindow.hpp \
src/gui/debugwindow.hpp \
src/util/util.hpp

SOURCES += \
src/core/bus.cpp \
src/core/cpu.cpp \
src/core/cartridge.cpp \
src/core/mapper/mapper.cpp \
src/core/mapper/mapper0.cpp \
src/gui/mainwindow.cpp \
src/gui/gamewindow.cpp \
src/gui/debugwindow.cpp \
src/util/util.cpp \
src/main.cpp

CONFIG += warn_off

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = moc