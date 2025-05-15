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
    gameWindow->setFixedSize(GAME_WIDTH, GAME_HEIGHT);

    debugWindow = new DebugWindow(nullptr);
    debugWindow->setFixedSize(DEBUG_WIDTH, GAME_HEIGHT);
    debugWindow->hide();

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addWidget(gameWindow);
    mainLayout->addWidget(debugWindow);

    QWidget* parentWidget = new QWidget();
    parentWidget->setLayout(mainLayout);
    setCentralWidget(parentWidget);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    controllerStatus[0].store(0, std::memory_order_relaxed);
    controllerStatus[1].store(0, std::memory_order_relaxed);
    resetFlag.store(false, std::memory_order_relaxed);
    debugWindowEnabled.store(false, std::memory_order_relaxed);
    stepModeEnabled.store(false, std::memory_order_relaxed);
    stepRequested.store(false, std::memory_order_relaxed);
    spritePallete.store(0, std::memory_order_relaxed);
    backgroundPallete.store(0, std::memory_order_relaxed);

    KeyboardInput keyInput = {
        &controllerStatus,
        &resetFlag,
        &debugWindowEnabled,
        &stepModeEnabled,
        &stepRequested,
        &spritePallete,
        &backgroundPallete
    };
    emulatorThread = new EmulatorThread(this, filePath, keyInput);

    connect(emulatorThread, &EmulatorThread::frameReadySignal, this, &MainWindow::displayNewFrame, Qt::QueuedConnection);
    connect(emulatorThread, &EmulatorThread::debugFrameReadySignal, this, &MainWindow::displayNewDebugFrame, Qt::QueuedConnection);

    emulatorThread->start();
}

void MainWindow::displayNewFrame(const QImage& image) {
    gameWindow->setCurrentFrame(image);
}

void MainWindow::displayNewDebugFrame(const DebugWindowState& state) {
    debugWindow->setCurrentState(state);
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
        resetFlag.store(true, std::memory_order_relaxed);
    }
    // Debugging keys
    else if (event->key() == Qt::Key_D) {
        if (debugWindow->isVisible()) {
            debugWindowEnabled.store(false, std::memory_order_relaxed);
            stepModeEnabled.store(0, std::memory_order_relaxed);
            debugWindow->hide();
        }
        else {
            debugWindowEnabled.store(true, std::memory_order_relaxed);
            debugWindow->show();
        }
    }
    else if (debugWindow->isVisible()) {
        if (event->key() == Qt::Key_Space) {
            stepRequested.store(true, std::memory_order_relaxed);
        }
        else if (event->key() == Qt::Key_C) {
            stepModeEnabled.fetch_xor(1, std::memory_order_relaxed);
        }
        else if (event->key() == Qt::Key_O) {
            backgroundPallete.fetch_add(1, std::memory_order_relaxed);
        }
        else if (event->key() == Qt::Key_P) {
            spritePallete.fetch_add(1, std::memory_order_relaxed);
        }
    }
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