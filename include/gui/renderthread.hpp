#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QTimer>

class RenderThread : public QThread {
    Q_OBJECT

public:
    RenderThread(QObject *parent, int fps);
    ~RenderThread();

signals:
    void triggerUpdate();

protected:
    void run() override;

private slots:
    void onTimeout();

private:
    const int FPS;
    QTimer* renderTimer;
};

#endif // RENDERTHREAD_H
