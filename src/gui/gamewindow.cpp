#include "gui/gamewindow.hpp"

#include "gui/mainwindow.hpp"

#include <QPainter>
#include <QPixmap>
#include <QImage>

GameWindow::GameWindow(QWidget* parent, const std::shared_ptr<Bus>& bus)
    : QWidget(parent), bus(bus), renderThread(new RenderThread(this)) {
    connect(renderThread, SIGNAL(triggerUpdate()), this, SLOT(onRenderUpdate()));
    renderThread->start();
}

GameWindow::~GameWindow() {
    renderThread->quit();
    renderThread->wait();
    renderThread->deleteLater();
}

void GameWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    const PPU::Display& display = *(bus->ppu->finishedDisplay);
    QImage image((uint8_t*)&display, 256, 240, QImage::Format::Format_ARGB32);
    const QPixmap pixmap = QPixmap::fromImage(image);
    painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
}

void GameWindow::onRenderUpdate() {
    update();
}