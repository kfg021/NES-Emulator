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

    uint8_t backgroundPallete;
    uint8_t spritePallete;

    static constexpr int NUM_INSTS = 9;

    QVector<QString> prevInsts;
    QString toString(uint16_t addr);

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:

    std::shared_ptr<Bus> bus;
};

#endif // DEBUGWINDOW_HPP