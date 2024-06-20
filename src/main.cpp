#include "gui/mainwindow.hpp"

#include <QApplication>

// Testing can be done using nestest.nes rom:
// path/to/NES/executable nestest.nes
int main(int argc, char* argv[]){
    QApplication app(argc, argv);

    std::string filePath = argc > 1 ? argv[1] : "";  
    MainWindow window(nullptr, filePath);
    window.show();

    return app.exec();
}