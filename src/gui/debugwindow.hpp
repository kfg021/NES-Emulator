#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include "../core/cpu.hpp"
#include "../core/cartridge.hpp"

#include <QWidget>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QTimer>

#include <memory>

class DebugWindow : public QWidget{
    Q_OBJECT

public:
    DebugWindow(QWidget* parent, const std::shared_ptr<Bus>& bus);
    void reset();
    void IRQ();
    void NMI();

    QTimer* updateTimer;
    QElapsedTimer* elapsedTimer;

    void toggleDebugMode();
    void stepIfInDebugMode();

    uint8_t colorPallete;

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    static constexpr int NUM_INSTS = 9;

    std::shared_ptr<Bus> bus;

    QVector<QString> prevInsts;
    QString toString(uint16_t addr);

    static constexpr int FPS = 60;
    static constexpr int IPS = 1700000;

    int64_t numSteps;
    int64_t numFrames;
    void tick();

    bool debugMode;

    void executeCycle();
    void executeInstruction();
};

#endif // DEBUGWINDOW_HPP