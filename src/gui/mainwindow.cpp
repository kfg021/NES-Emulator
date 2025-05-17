#include "gui/mainwindow.hpp"

#include "core/controller.hpp"
#include "core/cpu.hpp"
#include "gui/emulatorthread.hpp"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>

MainWindow::MainWindow(QWidget* parent, const std::string& filePath)
    : QMainWindow(parent) {

    setWindowTitle("NES Emulator");
    setFixedSize(GAME_WIDTH, GAME_HEIGHT);

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
    mainWindowData = image;
    update();
}

void MainWindow::displayNewDebugFrame(const DebugWindowState& state) {
    debugWindowData = state;

    // displayNewFrame will handle updates if emulator is running at full speed
    if (stepModeEnabled.load(std::memory_order_relaxed)) {
        update();
    }
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
        toggleDebugMode();
    }
    else if (debugWindowEnabled.load(std::memory_order_relaxed)) {
        if (event->key() == Qt::Key_Space) {
            if (stepModeEnabled.load(std::memory_order_relaxed)) {
                stepRequested.store(true, std::memory_order_relaxed);
            }
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

void MainWindow::toggleDebugMode() {
    bool debugMode = debugWindowEnabled.load(std::memory_order_relaxed);
    if (debugMode) {
        debugWindowEnabled.store(false, std::memory_order_relaxed);
        stepModeEnabled.store(0, std::memory_order_release);
        setFixedSize(GAME_WIDTH, GAME_HEIGHT);
    }
    else {
        debugWindowEnabled.store(true, std::memory_order_release);
        setFixedSize(TOTAL_WIDTH, GAME_HEIGHT);
    }
}

void MainWindow::paintEvent(QPaintEvent* /*event*/) {
    if (!mainWindowData.isNull()) {
        QPainter painter(this);
        const QPixmap pixmap = QPixmap::fromImage(mainWindowData);
        painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
    }

    if (debugWindowEnabled.load(std::memory_order_relaxed)) {
        renderDebugWindow();
    }
}

void MainWindow::renderDebugWindow() {
    static constexpr int X_OFFSET = 15;
    static constexpr int START_X = GAME_WIDTH + X_OFFSET;
    static constexpr int START_Y = 20;
    static constexpr int LETTER_HEIGHT = 20;

    QPainter painter(this);
    QColor defaultColor = painter.pen().color();
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(LETTER_HEIGHT);
    painter.setFont(font);
    int letterWidth = QFontMetrics(font).horizontalAdvance("A");

    const QString flagStr = "FLAGS:     ";
    painter.drawText(START_X, START_Y, flagStr);

    QString flagNames = "NV-BDIZC";
    for (int i = 0; i < 8; i++) {
        uint8_t sr = debugWindowData.sr;
        uint8_t flag = 7 - i;
        if ((sr >> flag) & 1) {
            painter.setPen(Qt::green);
        }
        else {
            painter.setPen(Qt::red);
        }

        QString flagStatusStr(1, flagNames[i]);
        painter.drawText(START_X + (flagStr.size() + i) * letterWidth, START_Y, flagStatusStr);
    }

    std::string pcStr = "PC:        $" + toHexString16(debugWindowData.pc);
    std::string aStr = "A:         $" + toHexString8(debugWindowData.a);
    std::string xStr = "X:         $" + toHexString8(debugWindowData.x);
    std::string yStr = "Y:         $" + toHexString8(debugWindowData.y);
    std::string spStr = "SP:        $" + toHexString8(debugWindowData.sp);

    painter.setPen(defaultColor);
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 2, QString(pcStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 3, QString(aStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 4, QString(xStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 5, QString(yStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 6, QString(spStr.c_str()));

    for (int i = 0; i < DebugWindowState::NUM_INSTS_TOTAL; i++) {
        if (i == DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW) {
            painter.setPen(Qt::cyan);
        }
        else {
            painter.setPen(defaultColor);
        }

        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (8 + i), debugWindowData.insts[i]);
    }

    const std::array<uint32_t, 0x20> palletes = debugWindowData.palleteRamColors;

    static constexpr int PALLETE_DISPLAY_SIZE = 7;
    static constexpr int NUM_INSTS = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW;

    {
        // Display each of the 4 background color palletes
        painter.setBrush(Qt::white);
        painter.drawRect(START_X + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.backgroundPallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

        for (int i = 0; i < 4; i++) {
            QImage image3(reinterpret_cast<const uint8_t*>(&palletes[i * 4]), 4, 1, QImage::Format::Format_ARGB32);
            const QPixmap palletePixmap = QPixmap::fromImage(image3);
            painter.drawPixmap(START_X + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
        }
        const PPU::PatternTable table1 = debugWindowData.table1;
        QImage image1(reinterpret_cast<const uint8_t*>(&table1), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
        const QPixmap pixmap1 = QPixmap::fromImage(image1);
        painter.drawPixmap(START_X, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap1);
    }

    {
        // Display each of the 4 sprite color palletes
        static constexpr int START_TABLE_2 = TOTAL_WIDTH - X_OFFSET - PPU::PATTERN_TABLE_SIZE;

        painter.setBrush(Qt::white);
        painter.drawRect(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.spritePallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

        for (int i = 0; i < 4; i++) {
            QImage image3(reinterpret_cast<const uint8_t*>(&palletes[0x10 + i * 4]), 4, 1, QImage::Format::Format_ARGB32);
            const QPixmap palletePixmap = QPixmap::fromImage(image3);
            painter.drawPixmap(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
        }

        const PPU::PatternTable table2 = debugWindowData.table2;
        QImage image2(reinterpret_cast<const uint8_t*>(&table2), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
        const QPixmap pixmap2 = QPixmap::fromImage(image2);
        painter.drawPixmap(START_TABLE_2, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap2);
    }
}