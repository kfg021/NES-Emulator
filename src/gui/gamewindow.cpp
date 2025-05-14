#include "gui/gamewindow.hpp"

#include "gui/mainwindow.hpp"

#include <QPainter>
#include <QPixmap>
#include <QImage>

GameWindow::GameWindow(QWidget* parent, const std::shared_ptr<Bus>& bus)
    : QWidget(parent),
    bus(bus),
    renderThread(new RenderThread(this, FPS)) {
    connect(renderThread, &RenderThread::triggerUpdate, this, &GameWindow::onRenderUpdate);
    renderThread->start();
}

GameWindow::~GameWindow() {
    renderThread->quit();
    renderThread->wait();
    renderThread->deleteLater();
}

void GameWindow::onRenderUpdate() {
    update();
}

void GameWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    const PPU::Display& display = *(bus->ppu->finishedDisplay);
    QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
    const QPixmap pixmap = QPixmap::fromImage(image);
    painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
}