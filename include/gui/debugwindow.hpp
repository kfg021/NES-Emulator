#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include "core/cpu.hpp"
#include "core/cartridge.hpp"
#include "gui/renderthread.hpp"

#include <memory>

#include <QKeyEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QWidget>

class DebugWindow : public QWidget {
    Q_OBJECT

public:
    DebugWindow(QWidget* parent, const std::shared_ptr<Bus>& bus);
    ~DebugWindow();

    void reset();

    uint8_t backgroundPallete;
    uint8_t spritePallete;

    static constexpr int NUM_INSTS = 9;

    QVector<QString> prevInsts;
    QString toString(uint16_t addr);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onRenderUpdate();

private:
    constexpr static int DEBUG_FPS = 15;
    std::shared_ptr<Bus> bus;
    RenderThread* renderThread;
};

#endif // DEBUGWINDOW_HPP