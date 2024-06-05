#include "mainwindow.hpp"

#include <QHBoxLayout>
#include <QPushButton>

#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent){
    std::string nesTest = "/Users/kennangumbs/Desktop/NES/nestest.nes";
    
    std::shared_ptr<Bus> bus = std::make_shared<Bus>();
    if(!bus->loadROM(nesTest)){
        qFatal("Could not load file");
    }

    setWindowTitle("NES Emulator");

    gameWindow = new GameWindow();
    gameWindow->setFixedSize(GAME_WIDTH, GAME_HEIGHT);

    debugWindow = new DebugWindow(nullptr, bus);
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