#include "gui/mainwindow.hpp"

#include "core/controller.hpp"
#include "core/cpu.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMediaDevices>

MainWindow::MainWindow(QWidget* parent, const std::string& filePath)
    : QMainWindow(parent) {

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

    numSteps = 0;
    scaledAudioClock = 0;

    globalMuteFlag = false;
    audioFormat.setSampleRate(AUDIO_SAMPLE_RATE);
    audioFormat.setChannelCount(1); // Mono
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioPlayer = new AudioPlayer(this, audioFormat, globalMuteFlag);

    audioSink = nullptr;
    resetAudioSink();

    mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged, this, &MainWindow::changeAudioOutputDevice);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::tick);
    updateTimer->setInterval(1000 / TICKS_PER_SECOND);

    elapsedTimer = QElapsedTimer();
    elapsedTimer.start();

#ifdef SHOW_DEBUG_WINDOW
    debugMode = true;
    toggleDebugMode();
#else
    updateTimer->start();
    elapsedTimer.start();
#endif
}

void MainWindow::tick() {
    int64_t totalElapsed = elapsedTimer.nsecsElapsed();

    int64_t neededSteps = ((totalElapsed * INSTRUCTIONS_PER_SECOND) / static_cast<int64_t>(1e9)) - numSteps;
    for (int i = 0; i < neededSteps; i++) {

#ifdef SHOW_DEBUG_WINDOW
        if (debugWindow->isVisible()) {
            executeCycleAndUpdateDebugWindow();
        }
        else {
            bus->executeCycle();
        }
#else
        bus->executeCycle();
#endif

        while (scaledAudioClock >= INSTRUCTIONS_PER_SECOND) {
            float sample = bus->apu->getAudioSample();

            audioPlayer->addSample(sample);

            scaledAudioClock -= INSTRUCTIONS_PER_SECOND;
        }

        numSteps++;
        scaledAudioClock += AUDIO_SAMPLE_RATE;
    }
}

void MainWindow::changeAudioOutputDevice() {
    audioPlayer->tryToMute(); // Mute while changing output device
    resetAudioSink();
    updateAudioState(); // This will unmute the audio player if needed
}

#ifdef SHOW_DEBUG_WINDOW
void MainWindow::toggleDebugMode() {
    debugMode ^= 1;

    if (!debugMode) {
        updateTimer->start();
        elapsedTimer.start();
        numSteps = 0;
    }
    else {
        updateTimer->stop();
    }

    updateAudioState();
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
    // TODO: This is technically this is not a reset. It is more like a "power on"
    bus->initDevices();

#ifdef SHOW_DEBUG_WINDOW
    debugWindow->reset();
#endif
}

// TODO: Key inputs for second controller
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

    // Toggle sound
    else if (event->key() == Qt::Key_S) {
        globalMuteFlag ^= 1;
        updateAudioState();
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


void MainWindow::updateAudioState() {
    auto tryToMute = [&]() -> void {
        if (audioSink->state() != QAudio::SuspendedState && audioSink->state() != QAudio::StoppedState) {
            audioSink->suspend();
        }
        audioPlayer->tryToMute();
    };

    auto tryToUnmute = [&]() -> void {
        audioPlayer->tryToUnmute();
        if (audioSink->state() == QAudio::SuspendedState) {
            audioSink->resume();
        }
    };

#ifdef SHOW_DEBUG_WINDOW
    bool shouldMute = globalMuteFlag || debugMode;
#else
    bool shouldMute = globalMuteFlag;
#endif

    if (shouldMute) {
        tryToMute();
    }
    else {
        tryToUnmute();
    }
}

void MainWindow::resetAudioSink() {
    if (audioSink != nullptr) {
        audioSink->stop();
        delete audioSink;
    }

    audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), audioFormat, this);
    audioSink->start(audioPlayer);
}