#include "gamewindow.hpp"

#include <QPainter>

GameWindow::GameWindow(QWidget* parent) : QWidget(parent){
}

void GameWindow::paintEvent(QPaintEvent* /*event*/){
    QPainter painter(this);
    painter.fillRect(rect(), Qt::blue);
}