#include "gui/renderthread.hpp"

#include "gui/mainwindow.hpp"

#include <QCoreApplication>

RenderThread::RenderThread(QObject* parent) 
    : QThread(parent), renderTimer(nullptr) {
}

RenderThread::~RenderThread() {
    renderTimer->deleteLater();
}

void RenderThread::run() {
    renderTimer = new QTimer();
    connect(renderTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    renderTimer->moveToThread(this);
    renderTimer->start(1000 / MainWindow::FPS);

    exec();
}

void RenderThread::onTimeout() {
    emit triggerUpdate();
}
