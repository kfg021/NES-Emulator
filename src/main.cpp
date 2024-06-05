// CONSOLE APPLICATION
// To test: 
// 1) g++ src/core/bus.cpp src/core/cartridge.cpp src/core/cpu.cpp src/core/ppu.cpp src/core/mapper/mapper.cpp src/core/mapper/mapper0.cpp src/main.cpp src/util/util.cpp
// 2) ./a.out > out.txt
// 3) diff out.txt nestestedit.log | awk -F, '/^[0-9]/ { print $1; exit }'
// 4) It should print 5004 (first encountered illegal opcode)
#include "core/cpu.hpp"
#include "core/bus.hpp"

#include <iostream>

int main(int argc, char* argv[]){
    std::string nesTest = "/Users/kennangumbs/Desktop/NES/nestest.nes";
        
    Bus bus{};
    if(!bus.loadROM(nesTest)){
        return 1;
    }

    for(int i = 0; i < 10000; i++){
        while(bus.cpu->getRemainingCycles()){
            bus.cpu->executeCycle();
        }
        bus.cpu->printDebug();
        bus.cpu->executeCycle();
    }

    return 0;
}

// GUI APPLICATION
// #include "gui/mainwindow.hpp"

// #include <QApplication>

// int main(int argc, char* argv[]){
//     QApplication app(argc, argv);

//     MainWindow window{};
//     window.show();

//     return app.exec();
// }