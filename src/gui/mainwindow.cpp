#include "gui/mainwindow.hpp"
#include "gui/emulatorthread.hpp"

#include "core/controller.hpp"
#include "core/cpu.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent, const std::string& filePath)
    : QMainWindow(parent) {
    setWindowTitle("NES Emulator");

    gameWindow = new GameWindow(nullptr);

#ifdef SHOW_DEBUG_WINDOW
    debugWindow = new DebugWindow(nullptr, bus);
    debugWindow->setFixedSize(DEBUG_WIDTH, GAME_HEIGHT);
    debugWindow->hide();
#endif

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addWidget(gameWindow);

#ifdef SHOW_DEBUG_WINDOW
    mainLayout->addWidget(debugWindow);
#endif

    QWidget* parentWidget = new QWidget();
    parentWidget->setLayout(mainLayout);
    setCentralWidget(parentWidget);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    controllerStatus[0].store(0, std::memory_order_relaxed);
    controllerStatus[1].store(0, std::memory_order_relaxed);

    resetFlag.store(false, std::memory_order_relaxed);

    GameInput gameInput{ &controllerStatus, &resetFlag };
    emulatorThread = new EmulatorThread(this, filePath, gameInput);

    connect(emulatorThread, &EmulatorThread::frameReadySignal, this, &MainWindow::displayNewFrame, Qt::QueuedConnection);

    emulatorThread->start();
}

void MainWindow::displayNewFrame(const PPU::Display& display) {
    gameWindow->setCurrentFrame(display);
}

// void MainWindow::tick() {
//     int64_t totalElapsed = elapsedTimer->nsecsElapsed();

//     int64_t neededSteps = ((totalElapsed * INSTRUCTIONS_PER_SECOND) / (int64_t)1e9) - numSteps;
//     for (int i = 0; i < neededSteps; i++) {
//         numSteps++;

// #ifdef SHOW_DEBUG_WINDOW
//         if (debugWindow->isVisible()) {
//             executeCycleAndUpdateDebugWindow();
//         }
//         else {
//             bus->executeCycle();
//         }
// #else
//         bus->executeCycle();
// #endif
//     }
// }

#ifdef SHOW_DEBUG_WINDOW
void MainWindow::toggleDebugMode() {
    debugMode ^= 1;

    if (!debugMode) {
        updateTimer->start();
        elapsedTimer->start();

        numSteps = 0;
    }
    else {
        updateTimer->stop();
    }
}

void MainWindow::stepIfInDebugMode() {
    if (debugMode) {
        // Pressing space causes the processor to jump to the next non-repeating instruction.
        // There is a maximum number of instructions to jump, preventing the emulator from crashing if there is an infinite loop in the code.
        static constexpr int MAX_LOOP = 1e5;
        uint16_t lastPC = bus->cpu->getPC();
        int i = 0;
        while (bus->cpu->getPC() == lastPC && i < MAX_LOOP) {
            executeInstruction();
            i++;
        }
    }
}

void MainWindow::executeInstruction() {
    while (bus->cpu->getRemainingCycles()) {
        bus->executeCycle();
    }

    // We should log this one
    executeCycleAndUpdateDebugWindow();
}

void MainWindow::executeCycleAndUpdateDebugWindow() {
    if (!bus->cpu->getRemainingCycles()) {
        QString currentInst = debugWindow->toString(bus->cpu->getPC());
        bus->executeCycle();

        if (debugWindow->prevInsts.size() == DebugWindow::NUM_INSTS) {
            debugWindow->prevInsts.pop_back();
        }
        debugWindow->prevInsts.prepend(currentInst);
    }
    else {
        bus->executeCycle();
    }
}
#endif

void MainWindow::reset() {
    // // TODO: This is technically this is not a reset. It is more like a "power on"
    // bus->initDevices();
    resetFlag.store(true, std::memory_order_relaxed);

#ifdef SHOW_DEBUG_WINDOW
    debugWindow->reset();
#endif
}

// TODO: Key inputs for second controller
void MainWindow::keyPressEvent(QKeyEvent* event) {
    // Game keys
    if (event->key() == Qt::Key_Up) {
        setControllerData(0, Controller::Button::UP, 1);
    }
    else if (event->key() == Qt::Key_Down) {
        setControllerData(0, Controller::Button::DOWN, 1);
    }
    else if (event->key() == Qt::Key_Left) {
        setControllerData(0, Controller::Button::LEFT, 1);
    }
    else if (event->key() == Qt::Key_Right) {
        setControllerData(0, Controller::Button::RIGHT, 1);
    }
    else if (event->key() == Qt::Key_Shift) {
        setControllerData(0, Controller::Button::SELECT, 1);
    }
    else if (event->key() == Qt::Key_Return) {
        setControllerData(0, Controller::Button::START, 1);
    }
    else if (event->key() == Qt::Key_Z) {
        setControllerData(0, Controller::Button::B, 1);
    }
    else if (event->key() == Qt::Key_X) {
        setControllerData(0, Controller::Button::A, 1);
    }

    // Reset button
    else if (event->key() == Qt::Key_R) {
        reset();
    }
#ifdef SHOW_DEBUG_WINDOW
    // Debugging keys
    else if (event->key() == Qt::Key_D) {
        if (debugWindow->isVisible()) {
            if (debugMode) {
                toggleDebugMode();
            }
            debugWindow->hide();
        }
        else {
            debugWindow->show();
        }
    }
    else if (debugWindow->isVisible()) {
        if (event->key() == Qt::Key_Space) {
            stepIfInDebugMode();
        }
        else if (event->key() == Qt::Key_C) {
            toggleDebugMode();
        }
        else if (event->key() == Qt::Key_O) {
            debugWindow->backgroundPallete = (debugWindow->backgroundPallete + 1) & 3;
        }
        else if (event->key() == Qt::Key_P) {
            debugWindow->spritePallete = (debugWindow->spritePallete + 1) & 3;
        }
    }

#endif
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    // Game keys
    if (event->key() == Qt::Key_Up) {
        setControllerData(0, Controller::Button::UP, 0);
    }
    else if (event->key() == Qt::Key_Down) {
        setControllerData(0, Controller::Button::DOWN, 0);
    }
    else if (event->key() == Qt::Key_Left) {
        setControllerData(0, Controller::Button::LEFT, 0);
    }
    else if (event->key() == Qt::Key_Right) {
        setControllerData(0, Controller::Button::RIGHT, 0);
    }
    else if (event->key() == Qt::Key_Shift) {
        setControllerData(0, Controller::Button::SELECT, 0);
    }
    else if (event->key() == Qt::Key_Return) {
        setControllerData(0, Controller::Button::START, 0);
    }
    else if (event->key() == Qt::Key_Z) {
        setControllerData(0, Controller::Button::B, 0);
    }
    else if (event->key() == Qt::Key_X) {
        setControllerData(0, Controller::Button::A, 0);
    }
}

void MainWindow::setControllerData(bool controller, Controller::Button button, bool value) {
    uint8_t bitToChange = (1 << static_cast<int>(button));
    if (value) {
        controllerStatus[controller].fetch_or(bitToChange, std::memory_order_relaxed);
    }
    else {
        controllerStatus[controller].fetch_and(~bitToChange, std::memory_order_relaxed);
    }
}