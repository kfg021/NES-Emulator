// CONSOLE APPLICATION
// To test: 
// 1) src/core/bus.cpp src/core/cpu.cpp src/core/cartridge.cpp src/core/mapper/mapper.cpp src/core/mapper/mapper0.cpp src/main.cpp src/util/util.cpp
// 2) ./a.out > out.txt
// 3) diff out.txt nestestedit.log | awk -F, '/^[0-9]/ { print $1; exit }'
// 4) It should print 5004 (first encountered illegal opcode)
// #include "core/cpu.hpp"
// #include "core/bus.hpp"

// #include <iostream>

// int main(int argc, char* argv[]){
//     std::string nesTest = "/Users/kennangumbs/Desktop/NES/nestest.nes";

//     std::shared_ptr<Cartridge> cartridge = std::make_shared<Cartridge>(nesTest);
//     if(!cartridge->isValid()){
//         return 1;
//     }
        
//     std::shared_ptr<Bus> bus = std::make_shared<Bus>(cartridge);
//     std::shared_ptr<CPU> cpu = std::make_shared<CPU>();
//     cpu->setBus(bus);
//     cpu->initCPU();

//     for(int i = 0; i < 10000; i++){
//         while(cpu->getRemainingCycles()){
//             cpu->executeCycle();
//         }
//         cpu->printDebug();
//         cpu->executeCycle();
//     }

//     return 0;
// }

// GUI APPLICATION
#include "gui/mainwindow.hpp"

#include <QApplication>

int main(int argc, char* argv[]){
    QApplication app(argc, argv);

    MainWindow window{};
    window.show();

    return app.exec();
}