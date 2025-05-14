#include "gui/renderthread.hpp"

#include "gui/mainwindow.hpp"

#include <QCoreApplication>

RenderThread::RenderThread(QObject* parent, int fps)
    : QThread(parent),
    renderTimer(nullptr),
    FPS(fps) {
}

RenderThread::~RenderThread() {
    renderTimer->deleteLater();
}

void RenderThread::run() {
    renderTimer = new QTimer();
    connect(renderTimer, &QTimer::timeout, this, &RenderThread::onTimeout);
    renderTimer->moveToThread(this);
    renderTimer->start(1000 / FPS);

    exec();
}

void RenderThread::onTimeout() {
    emit triggerUpdate();
}
