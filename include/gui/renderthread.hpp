#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QTimer>

class RenderThread : public QThread {
    Q_OBJECT

public:
    RenderThread(QObject *parent = nullptr);
    ~RenderThread();

signals:
    void triggerUpdate();

protected:
    void run() override;

private slots:
    void onTimeout();

private:
    QTimer* renderTimer;
};

#endif // RENDERTHREAD_H
