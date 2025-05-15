#include "gui/gamewindow.hpp"

#include "gui/mainwindow.hpp"

#include <QImage>
#include <QPainter>
#include <QPixmap>

GameWindow::GameWindow(QWidget* parent)
    : QWidget(parent) {
}

void GameWindow::setCurrentFrame(const QImage& image) {
    currentFrame = image;
    update();
}

void GameWindow::paintEvent(QPaintEvent* /*event*/) {
    if (!currentFrame.isNull()) {
        QPainter painter(this);
        const QPixmap pixmap = QPixmap::fromImage(currentFrame);
        painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
    }
}