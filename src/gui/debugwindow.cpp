#include "debugwindow.hpp"

#include "../util/util.hpp"

#include <QPainter>
#include <QTimer>
#include <QFont>
#include <QFontDatabase>
#include <Qstring>
#include <QColor>

DebugWindow::DebugWindow(QWidget* parent, const std::shared_ptr<Bus>& bus) :
    QWidget(parent), 
    bus(bus){
}

void DebugWindow::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);

    const static int LETTER_HEIGHT = 20;
    const static int START_X = 10;
    const static int START_Y = 20;

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
}


void DebugWindow::executeInstruction(){
    QString currentInst = toString(bus->cpu->getPC());
    bus->cpu->executeNextInstruction();

    if(prevInsts.size() == NUM_INSTS){
        prevInsts.pop_back();
    }
    prevInsts.prepend(currentInst);

    update();
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