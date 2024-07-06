#include "gui/debugwindow.hpp"

#include "util/util.hpp"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QString>
#include <QTimer>


DebugWindow::DebugWindow(QWidget* parent, const std::shared_ptr<Bus>& bus)
    : QWidget(parent), bus(bus), renderThread(new RenderThread(this)) {
    connect(renderThread, SIGNAL(triggerUpdate()), this, SLOT(onRenderUpdate()));
    renderThread->start();

    backgroundPallete = 0;
    spritePallete = 0;
}

DebugWindow::~DebugWindow() {
    renderThread->quit();
    renderThread->wait();
    renderThread->deleteLater();
}

void DebugWindow::onRenderUpdate() {
    update();
}

void DebugWindow::paintEvent(QPaintEvent* /*event*/) {
    static constexpr int LETTER_HEIGHT = 20;
    static constexpr int START_X = 10;
    static constexpr int START_Y = 20;

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
        uint8_t sr = bus->cpu->getSR();
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

    std::string pcStr = "PC:        $" + toHexString16(bus->cpu->getPC());
    std::string aStr = "A:         $" + toHexString8(bus->cpu->getA());
    std::string xStr = "X:         $" + toHexString8(bus->cpu->getX());
    std::string yStr = "Y:         $" + toHexString8(bus->cpu->getY());
    std::string spStr = "SP:        $" + toHexString8(bus->cpu->getSP());

    painter.setPen(defaultColor);
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 2, QString(pcStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 3, QString(aStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 4, QString(xStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 5, QString(yStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 6, QString(spStr.c_str()));

    for (int i = 0; i < prevInsts.size(); i++) {
        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (7 + NUM_INSTS - i), prevInsts[i]);
    }

    painter.setPen(Qt::cyan);
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * (8 + NUM_INSTS), toString(bus->cpu->getPC()));
    painter.setPen(defaultColor);

    uint16_t nextPC = bus->cpu->getPC();
    for (int i = 0; i < NUM_INSTS; i++) {
        const CPU::Opcode& op = bus->cpu->getOpcode(nextPC);
        nextPC += op.addressingMode.instructionSize;
        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (9 + NUM_INSTS + i), toString(nextPC));
    }

    std::array<uint32_t, 0x20> palletes = bus->ppu->getPalleteRamColors();

    static constexpr int PALLETE_DISPLAY_SIZE = 7;

    {
        // Display each of the 4 background color palletes
        painter.setBrush(Qt::white);
        painter.drawRect(START_X + (PALLETE_DISPLAY_SIZE * 5) * backgroundPallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

        for (int i = 0; i < 4; i++) {
            QImage image3((uint8_t*)&palletes[i * 4], 4, 1, QImage::Format::Format_ARGB32);
            const QPixmap palletePixmap = QPixmap::fromImage(image3);
            painter.drawPixmap(START_X + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
        }
        PPU::PatternTable table1 = bus->ppu->getPatternTable(true, backgroundPallete);
        QImage image1((uint8_t*)&table1, PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
        const QPixmap pixmap1 = QPixmap::fromImage(image1);
        painter.drawPixmap(START_X, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap1);
    }

    {
        // Display each of the 4 sprite color palletes
        int start = width() - START_X - PPU::PATTERN_TABLE_SIZE;

        painter.setBrush(Qt::white);
        painter.drawRect(start + (PALLETE_DISPLAY_SIZE * 5) * spritePallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

        for (int i = 0; i < 4; i++) {
            QImage image3((uint8_t*)&palletes[0x10 + i * 4], 4, 1, QImage::Format::Format_ARGB32);
            const QPixmap palletePixmap = QPixmap::fromImage(image3);
            painter.drawPixmap(start + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
        }

        PPU::PatternTable table2 = bus->ppu->getPatternTable(false, spritePallete);
        QImage image2((uint8_t*)&table2, PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
        const QPixmap pixmap2 = QPixmap::fromImage(image2);
        painter.drawPixmap(start, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap2);
    }
}

void DebugWindow::reset() {
    prevInsts.clear();
    update();
}

QString DebugWindow::toString(uint16_t addr) {
    std::string text = "$" + toHexString16(addr) + ": " + bus->cpu->toString(addr);
    return QString(text.c_str());
}