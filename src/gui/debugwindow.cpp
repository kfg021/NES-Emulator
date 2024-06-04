#include "debugwindow.hpp"

#include "../util/util.hpp"

#include <QPainter>
#include <QTimer>
#include <QFont>
#include <QFontDatabase>
#include <Qstring>
#include <QColor>

DebugWindow::DebugWindow(QWidget* parent, const std::shared_ptr<CPU>& cpu) : QWidget(parent), cpu(cpu) {
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
        uint8_t sr = cpu->getSR();
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

    std::string pcStr = "PC:        $"  + toHexString16(cpu->getPC());
    std::string aStr  = "A:         $"  + toHexString8(cpu->getA());
    std::string xStr  = "X:         $"  + toHexString8(cpu->getX());
    std::string yStr  = "Y:         $"  + toHexString8(cpu->getY());
    std::string spStr = "SP:        $"  + toHexString8(cpu->getSP());

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
    painter.drawText(START_X, START_Y + LETTER_HEIGHT * (8 + NUM_INSTS), toString(cpu->getPC()));
    painter.setPen(defaultColor);

    uint16_t nextPC = cpu->getPC();
    for(int i = 0; i < NUM_INSTS; i++){
        const CPU::Opcode& op = cpu->getOpcode(nextPC);
        nextPC += op.addressingMode.instructionSize;
        painter.drawText(START_X, START_Y + LETTER_HEIGHT * (9 + NUM_INSTS + i), toString(nextPC));
    }
}


void DebugWindow::executeInstruction(){
    QString currentInst = toString(cpu->getPC());
    cpu->executeNextInstruction();

    if(prevInsts.size() == NUM_INSTS){
        prevInsts.pop_back();
    }
    prevInsts.prepend(currentInst);

    update();
}

void DebugWindow::reset(){
    cpu->reset();
    prevInsts.clear();
    update();
}

void DebugWindow::IRQ(){
    if(cpu->IRQ()){
        prevInsts.clear();
        update();
    }
}

void DebugWindow::NMI(){
    cpu->NMI();
    prevInsts.clear();
    update();
}

QString DebugWindow::toString(uint16_t addr){
    std::string text = "$" + toHexString16(addr) + ": " +  cpu->toString(addr);
    return QString(text.c_str());
}