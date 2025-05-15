#include "gui/gamewindow.hpp"

#include <QPainter>
#include <QPixmap>
#include <QImage>

GameWindow::GameWindow(QWidget* parent)
    : QWidget(parent) {
    this->setFixedSize(GAME_WIDTH, GAME_HEIGHT);
}

void GameWindow::setCurrentFrame(const PPU::Display& display) {
    QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
    currentFrame = image.copy();
    update();
}

void GameWindow::paintEvent(QPaintEvent* /*event*/) {
    if (!currentFrame.isNull()) {
        QPainter painter(this);
        const QPixmap pixmap = QPixmap::fromImage(currentFrame);
        painter.drawPixmap(0, 0, GAME_WIDTH, GAME_HEIGHT, pixmap);
    }
}