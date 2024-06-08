#include "debugwindow.hpp"

#include "../util/util.hpp"

#include <QPainter>
#include <QTimer>
#include <QFont>
#include <QFontDatabase>
#include <QString>
#include <QColor>

DebugWindow::DebugWindow(QWidget* parent, const std::shared_ptr<Bus>& bus) :
    QWidget(parent), 
    bus(bus){
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &DebugWindow::tick);
    updateTimer->setInterval(1000 / FPS);

    numSteps = 0;
    numFrames = 0;
    debugMode = true;
    colorPallete = 0;

    elapsedTimer = new QElapsedTimer();
    elapsedTimer->start();
}

void DebugWindow::toggleDebugMode(){
    debugMode ^= 1;

    if(!debugMode){
        updateTimer->start();
        elapsedTimer->start();

        numSteps = 0;
        numFrames = 0;
    }
    else{
        updateTimer->stop();
    }
    
}

void DebugWindow::stepIfInDebugMode(){
    if(debugMode){
        executeInstruction();
        update();
    }
}

void DebugWindow::tick(){
    int64_t totalElapsed = elapsedTimer->nsecsElapsed();

    // step
    int64_t neededSteps = ((totalElapsed * IPS) / (int64_t)1e9) - numSteps;
    for(int i = 0; i < neededSteps; i++){
        numSteps++;
        executeCycle();
    }

    // draw
    int64_t neededFrames = ((totalElapsed * FPS) / (int64_t)1e9) - numFrames;
    for(int i = 0; i < neededFrames; i++){
        numFrames++;
        update();
    }
} 

void DebugWindow::paintEvent(QPaintEvent* /*event*/){
    static constexpr int LETTER_HEIGHT = 20;
    static constexpr int START_X = 10;
    static constexpr int START_Y = 20;

    QPainter painter(this);
    QColor defaultColor = painter.pen().color();
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(LETTER_HEIGHT);
    painter.setFont(font);

    QFontMetrics metrics(font);
    int letterWidth = metrics.horizontalAdvance("A");

    const QString flagStr = "FLAGS:     ";
    painter.drawText(START_X, START_Y, flagStr);

    QString flagNames = "NV-BDIZC";
    for(int i = 0; i < 8; i++){
        uint8_t sr = bus->cpu->getSR();
        uint8_t flag = 7-i;
        if((sr >> flag) & 1){
            painter.setPen(Qt::green);
        }
        else{
            painter.setPen(Qt::red);
        }

        QString flagStatusStr(1, flagNames[i]);
        painter.drawText(START_X + (flagStr.size() + i) * letterWidth, START_Y, flagStatusStr);
    }

    std::string pcStr = "PC:        $"  + toHexString16(bus->cpu->getPC());
    std::string aStr  = "A:         $"  + toHexString8(bus->cpu->getA());
    std::string xStr  = "X:         $"  + toHexString8(bus->cpu->getX());
    std::string yStr  = "Y:         $"  + toHexString8(bus->cpu->getY());
    std::string spStr = "SP:        $"  + toHexString8(bus->cpu->getSP());

    painter.setPen(defaultColor);
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 2, QString(pcStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 3, QString(aStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 4, QString(xStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 5, QString(yStr.c_str()));
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * 6, QString(spStr.c_str()));
    
    for(int i = 0; i < prevInsts.size(); i++){
        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (7 + NUM_INSTS - i), prevInsts[i]);
    }

    painter.setPen(Qt::cyan);
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * (8 + NUM_INSTS), toString(bus->cpu->getPC()));
    painter.setPen(defaultColor);

    uint16_t nextPC = bus->cpu->getPC();
    for(int i = 0; i < NUM_INSTS; i++){
        const CPU::Opcode& op = bus->cpu->getOpcode(nextPC);
        nextPC += op.addressingMode.instructionSize;
        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (9 + NUM_INSTS + i), toString(nextPC));
    }

    std::array<uint32_t, 0x20> palletes = bus->ppu->getPalleteRamColors();

    static constexpr int PALLETE_DISPLAY_SIZE = 7;
    
    // Draw border around selected pallete
    painter.setBrush(Qt::white);
    painter.drawRect(START_X + (PALLETE_DISPLAY_SIZE * 5) * colorPallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);
    
    // Display each of the 8 color palletes
    for(int i = 0; i < 8; i++){
        QImage image3((uint8_t*)&palletes[i * 4], 4, 1, QImage::Format::Format_ARGB32);
        const QPixmap palletePixmap = QPixmap::fromImage(image3);
        painter.drawPixmap(START_X + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
    }

    PPU::PatternTable table1 = bus->ppu->getPatternTable(0, colorPallete);
    QImage image1((uint8_t*)&table1, PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
    const QPixmap pixmap1 = QPixmap::fromImage(image1);
    painter.drawPixmap(START_X, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap1);

    PPU::PatternTable table2 = bus->ppu->getPatternTable(1, colorPallete);
    QImage image2((uint8_t*)&table2, PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
    const QPixmap pixmap2 = QPixmap::fromImage(image2);
    painter.drawPixmap(width() - START_X - PPU::PATTERN_TABLE_SIZE, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap2);
}

void DebugWindow::executeCycle(){
    if(!bus->cpu->getRemainingCycles()){
        QString currentInst = toString(bus->cpu->getPC());
        
        bus->executeCycle();

        if(prevInsts.size() == NUM_INSTS){
            prevInsts.pop_back();
        }
        prevInsts.prepend(currentInst);
    }
    else{
        bus->executeCycle();
    }
}

void DebugWindow::executeInstruction(){
    while(bus->cpu->getRemainingCycles()){
        bus->executeCycle();
    }
    
    executeCycle();
}

void DebugWindow::reset(){
    bus->cpu->reset();
    prevInsts.clear();
    update();
}

void DebugWindow::IRQ(){
    if(bus->cpu->IRQ()){
        prevInsts.clear();
        update();
    }
}

void DebugWindow::NMI(){
    bus->cpu->NMI();
    prevInsts.clear();
    update();
}

QString DebugWindow::toString(uint16_t addr){
    std::string text = "$" + toHexString16(addr) + ": " +  bus->cpu->toString(addr);
    return QString(text.c_str());
}