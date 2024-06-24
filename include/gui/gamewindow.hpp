#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "core/bus.hpp"

#include <memory>

#include <QPaintEvent>
#include <QWidget>

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget* parent, const std::shared_ptr<Bus>& bus);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::shared_ptr<Bus> bus;
};

#endif // GAMEWINDOW_HPP