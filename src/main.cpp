// CONSOLE APPLICATION
// To test: 
// 1) Uncomment write16BitData(RESET_VECTOR, 0xC000) in the initCPU() method in cpu.cpp
// 2) g++ src/core/bus.cpp src/core/cartridge.cpp src/core/controller.cpp src/core/cpu.cpp src/core/ppu.cpp src/core/mapper/mapper.cpp src/core/mapper/mapper0.cpp src/main.cpp src/util/util.cpp
// 3) ./a.out nestest.nes > out.txt
// 4) diff out.txt nestestedit.log | awk -F, '/^[0-9]/ { print $1; exit }'
// 5) It should print 5004 (first encountered illegal opcode)
// TODO: add PPU scanlines/cycles to test
// #include "core/bus.hpp"
// #include "core/cpu.hpp"

// #include <iostream>

// int main(int argc, char* argv[]){
//     std::string filePath = argc > 1 ? argv[1] : "";    
        
//     Bus bus{};
//     if(!bus.loadROM(filePath)){
//         return 1;
//     }

//     for(int i = 0; i < 10000; i++){
//         while(bus.cpu->getRemainingCycles()){
//             bus.executeCycle();
//         }
        
//         bus.cpu->printDebug();
        
//         // 3 bus cycles = 1 cpu cycle
//         for(int j = 0; j < 3; j++){
//             bus.executeCycle();
//         }
//     }

//     return 0;
// }

// GUI APPLICATION
#include "gui/mainwindow.hpp"

#include <QApplication>

int main(int argc, char* argv[]){
    QApplication app(argc, argv);

    std::string filePath = argc > 1 ? argv[1] : "";  
    MainWindow window(nullptr, filePath);
    window.show();

    return app.exec();
}