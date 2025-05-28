#include "io/mainwindow.hpp"

#include <QApplication>
#include <QDir>
#include <QFileDialog>

// Testing can be done using nestest.nes rom:
// path/to/NES/executable nestest.nes
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Choose the .nes file to run.
    // Use input from the command line argument if provided, otherwise choose from file dialog
    std::string filePath;
    if (argc > 1) {
        filePath = argv[1];
    }
    else {
        filePath = QFileDialog::getOpenFileName(nullptr, "Choose a .nes file to open.", QDir::homePath(), "(*.nes)").toStdString();
        if (filePath.empty()) {
            // Exit if file dialog is closed without selecting a file
            return 0;
        }
    }

    MainWindow window(nullptr, filePath);
    window.show();

    return app.exec();
}