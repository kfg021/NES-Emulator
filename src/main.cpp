// CONSOLE APPLICATION
// To test: 
// 1) g++ src/main.cpp src/core/bus.cpp src/core/cpu.cpp src/util/util.cpp
// 2) ./a.out > out.txt
// 3) diff out.txt nestestedit.log | awk -F, '/^[0-9]/ { print $1; exit }'
// 4) It should print 5004 (first encountered illegal opcode)
// #include "core/cpu.hpp"
// #include "core/bus.hpp"

// #include <iostream>

// int main(int argc, char* argv[]){
//     CPU cpu(std::make_shared<Bus>());

//     std::string nesTest = "nestest.nes";
//     static const int len = 0xC000 - 0x8000;
//     cpu.load(nesTest, 0x8000, len, 0x10);
//     cpu.load(nesTest, 0xC000, len, 0x10);

//     for(int i = 0; i < 10000; i++){
//         while(cpu.getRemainingCycles()){
//             cpu.executeCycle();
//         }
//         cpu.printDebug();
//         cpu.executeCycle();
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