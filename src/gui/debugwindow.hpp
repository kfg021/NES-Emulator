#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include "../core/cpu.hpp"

#include <QWidget>
#include <QKeyEvent>
#include <QPaintEvent>

#include <memory>

class DebugWindow : public QWidget{
    Q_OBJECT

public:
    DebugWindow(QWidget* parent, const std::shared_ptr<CPU>& cpu);
    void executeInstruction();
    void reset();
    void IRQ();
    void NMI();

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    const static int NUM_INSTS = 12;

    std::shared_ptr<CPU> cpu;
    QTimer* updateTimer;
    QVector<QString> prevInsts;
    QString toString(uint16_t addr);
};

#endif // DEBUGWINDOW_HPP