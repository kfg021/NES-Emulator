#include "mainwindow.hpp"

#include <QHBoxLayout>
#include <QPushButton>

#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent){
    cpu = std::make_shared<CPU>(std::make_shared<Bus>());

    std::string nesTest = "/Users/kennangumbs/Desktop/NES/nestest.nes";
    static const uint16_t len = 0xC000 - 0x8000;
    bool load1 = cpu->load(nesTest, 0x8000, len, 0x10);
    bool load2 = cpu->load(nesTest, 0xC000, len, 0x10);

    if(!(load1 && load2)){
        qFatal("Could not load file");
    }

    setWindowTitle("NES Emulator");

    gameWindow = new GameWindow();
    gameWindow->setFixedSize(GAME_WIDTH, GAME_HEIGHT);

    debugWindow = new DebugWindow(nullptr, cpu);
    debugWindow->setFixedSize(DEBUG_WIDTH, GAME_HEIGHT);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(gameWindow);
    layout->addWidget(debugWindow);

    QWidget* parentWidget = new QWidget();
    parentWidget->setLayout(layout);
    setCentralWidget(parentWidget);
}

void MainWindow::keyPressEvent(QKeyEvent* event){
    if(event->key() == Qt::Key_Space){
        debugWindow->executeInstruction();
    }
    else if(event->key() == Qt::Key_R){
        debugWindow->reset();
    }
    else if(event->key() == Qt::Key_I){
        debugWindow->IRQ();
    }
    else if(event->key() == Qt::Key_N){
        debugWindow->NMI();
    }
}