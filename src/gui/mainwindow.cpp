#include "gui/mainwindow.hpp"

#include "core/controller.hpp"
#include "core/cpu.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent, const std::string& filePath) : QMainWindow(parent) {
    bus = std::make_shared<Bus>();

    Cartridge::Status status = bus->loadROM(filePath);
    if (status.code != Cartridge::Code::SUCCESS) {
        qFatal("%s", status.message.c_str());
    }

    setWindowTitle("NES Emulator");

    gameWindow = new GameWindow(nullptr, bus);
    gameWindow->setFixedSize(GAME_WIDTH, GAME_HEIGHT);

#ifdef SHOW_DEBUG_WINDOW
    debugWindow = new DebugWindow(nullptr, bus);
    debugWindow->setFixedSize(DEBUG_WIDTH, GAME_HEIGHT);
#endif

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(gameWindow);

#ifdef SHOW_DEBUG_WINDOW
    layout->addWidget(debugWindow);
#endif

    QWidget* parentWidget = new QWidget();
    parentWidget->setLayout(layout);
    setCentralWidget(parentWidget);

    numSteps = 0;
    numFrames = 0;

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::tick);
    updateTimer->setInterval(1000 / FPS);

    elapsedTimer = new QElapsedTimer();
    elapsedTimer->start();

#ifdef SHOW_DEBUG_WINDOW
    debugMode = true;
    toggleDebugMode();
#else
    updateTimer->start();
    elapsedTimer->start();
#endif
}

#ifdef SHOW_DEBUG_WINDOW
void MainWindow::toggleDebugMode() {
    debugMode ^= 1;

    if (!debugMode) {
        updateTimer->start();
        elapsedTimer->start();

        numSteps = 0;
        numFrames = 0;
    }
    else {
        updateTimer->stop();

        gameWindow->update();
        debugWindow->update();
    }
}
#endif

#ifdef SHOW_DEBUG_WINDOW
void MainWindow::stepIfInDebugMode() {
    if (debugMode) {
        executeInstruction();

        gameWindow->update();

        debugWindow->update();
    }
}
#endif

void MainWindow::tick() {
    int64_t totalElapsed = elapsedTimer->nsecsElapsed();

    // step
    int64_t neededSteps = ((totalElapsed * IPS) / (int64_t)1e9) - numSteps;
    for (int i = 0; i < neededSteps; i++) {
        numSteps++;

        executeCycle();
    }

    // draw
    int64_t neededFrames = ((totalElapsed * FPS) / (int64_t)1e9) - numFrames;
    for (int i = 0; i < neededFrames; i++) {
        numFrames++;
        gameWindow->update();

#ifdef SHOW_DEBUG_WINDOW
        debugWindow->update();
#endif
    }
}

void MainWindow::executeCycle() {
    if (!bus->cpu->getRemainingCycles()) {

#ifdef SHOW_DEBUG_WINDOW
        QString currentInst = debugWindow->toString(bus->cpu->getPC());
#endif

        bus->executeCycle();

#ifdef SHOW_DEBUG_WINDOW
        if (debugWindow->prevInsts.size() == DebugWindow::NUM_INSTS) {
            debugWindow->prevInsts.pop_back();
        }
        debugWindow->prevInsts.prepend(currentInst);
#endif
    }
    else {
        bus->executeCycle();
    }
}

#ifdef SHOW_DEBUG_WINDOW
void MainWindow::executeInstruction() {
    while (bus->cpu->getRemainingCycles()) {
        bus->executeCycle();
    }

    bus->executeCycle();
    bus->executeCycle();

    // We should log this one
    executeCycle();
}
#endif

void MainWindow::reset() {
    // TODO: Technically this is not a reset. It is more like a "power on"
    bus->initDevices();
    gameWindow->update();

#ifdef SHOW_DEBUG_WINDOW
    debugWindow->reset();
#endif
}

// TODO: Only one controller for now
void MainWindow::keyPressEvent(QKeyEvent* event) {
    // Game keys
    if (event->key() == Qt::Key_Up) {
        bus->setController(0, Controller::Button::UP, 1);
    }
    else if (event->key() == Qt::Key_Down) {
        bus->setController(0, Controller::Button::DOWN, 1);
    }
    else if (event->key() == Qt::Key_Left) {
        bus->setController(0, Controller::Button::LEFT, 1);
    }
    else if (event->key() == Qt::Key_Right) {
        bus->setController(0, Controller::Button::RIGHT, 1);
    }
    else if (event->key() == Qt::Key_Shift) {
        bus->setController(0, Controller::Button::SELECT, 1);
    }
    else if (event->key() == Qt::Key_Return) {
        bus->setController(0, Controller::Button::START, 1);
    }
    else if (event->key() == Qt::Key_Z) {
        bus->setController(0, Controller::Button::B, 1);
    }
    else if (event->key() == Qt::Key_X) {
        bus->setController(0, Controller::Button::A, 1);
    }

    // Reset button
    else if (event->key() == Qt::Key_R) {
        reset();
    }

#ifdef SHOW_DEBUG_WINDOW
    // Debugging keys
    else if (event->key() == Qt::Key_Space) {
        stepIfInDebugMode();
    }
    else if (event->key() == Qt::Key_C) {
        toggleDebugMode();
    }
    else if (event->key() == Qt::Key_O) {
        debugWindow->backgroundPallete = (debugWindow->backgroundPallete + 1) & 3;
        debugWindow->update();
    }
    else if (event->key() == Qt::Key_P) {
        debugWindow->spritePallete = (debugWindow->spritePallete + 1) & 3;
        debugWindow->update();
    }
#endif
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    // Game keys
    if (event->key() == Qt::Key_Up) {
        bus->setController(0, Controller::Button::UP, 0);
    }
    else if (event->key() == Qt::Key_Down) {
        bus->setController(0, Controller::Button::DOWN, 0);
    }
    else if (event->key() == Qt::Key_Left) {
        bus->setController(0, Controller::Button::LEFT, 0);
    }
    else if (event->key() == Qt::Key_Right) {
        bus->setController(0, Controller::Button::RIGHT, 0);
    }
    else if (event->key() == Qt::Key_Shift) {
        bus->setController(0, Controller::Button::SELECT, 0);
    }
    else if (event->key() == Qt::Key_Return) {
        bus->setController(0, Controller::Button::START, 0);
    }
    else if (event->key() == Qt::Key_Z) {
        bus->setController(0, Controller::Button::B, 0);
    }
    else if (event->key() == Qt::Key_X) {
        bus->setController(0, Controller::Button::A, 0);
    }
}