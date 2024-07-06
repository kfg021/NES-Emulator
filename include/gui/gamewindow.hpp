#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include "core/bus.hpp"
#include "gui/renderthread.hpp"

#include <memory>

#include <QWidget>

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget *parent, const std::shared_ptr<Bus>& bus);
    ~GameWindow();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onRenderUpdate();

private:
    std::shared_ptr<Bus> bus;
    RenderThread* renderThread;
};

#endif // GAMEWINDOW_H
