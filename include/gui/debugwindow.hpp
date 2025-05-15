#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include "core/cpu.hpp"
#include "core/cartridge.hpp"
#include "gui/guitypes.hpp"

#include <memory>

#include <QKeyEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QWidget>

class DebugWindow : public QWidget {
    Q_OBJECT
public:
    DebugWindow(QWidget* parent);
    void setCurrentState(const DebugWindowState& state);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    DebugWindowState state;
    // QString toString(uint16_t addr);
};

#endif // DEBUGWINDOW_HPP