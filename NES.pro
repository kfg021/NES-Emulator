QT = core gui widgets

HEADERS += \
src/core/bus.hpp \
src/core/cartridge.hpp \
src/core/cpu.hpp \
src/core/ppu.hpp \
src/core/mapper/mapper.hpp \
src/core/mapper/mapper0.hpp \
src/gui/debugwindow.hpp \
src/gui/gamewindow.hpp \
src/gui/mainwindow.hpp \
src/util/util.hpp

SOURCES += \
src/core/bus.cpp \
src/core/cartridge.cpp \
src/core/cpu.cpp \
src/core/ppu.cpp \
src/core/mapper/mapper.cpp \
src/core/mapper/mapper0.cpp \
src/gui/debugwindow.cpp \
src/gui/gamewindow.cpp \
src/gui/mainwindow.cpp \
src/util/util.cpp \
src/main.cpp

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = moc