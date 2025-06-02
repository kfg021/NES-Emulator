#include "io/mainwindow.hpp"

#include <optional>

#include <QApplication>
#include <QDir>
#include <QFileDialog>

// Testing can be done using nestest.nes rom:
// path/to/NES/executable nestest.nes
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Choose the .nes file to run.
    // Use input from the command line argument if provided, otherwise choose from file dialog
    // The second command line argument is optional and starts the ROM from a save state (.sstate file)
    std::string romFilePath;
    std::optional<std::string> saveFilePath;
    switch (argc) {
        case 1:
            romFilePath = QFileDialog::getOpenFileName(nullptr, "Choose a .nes file to open.", QDir::homePath(), "(*.nes)").toStdString();
            if (romFilePath.empty()) {
                qFatal("No ROM file selected.");
            }
            break;

        case 2:
            romFilePath = argv[1];
            break;

        case 3:
            romFilePath = argv[1];
            saveFilePath = argv[2];
            break;

        default:
            qFatal("Incorrect number of command line arguments given.");
            break;
    }

    MainWindow window(nullptr, romFilePath, saveFilePath);
    window.show();

    return app.exec();
}