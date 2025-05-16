#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "core/bus.hpp"
#include "gui/audioplayer.hpp"
#include "gui/debugwindow.hpp"
#include "gui/emulatorthread.hpp"
#include "gui/guitypes.hpp"
#include "gui/gamewindow.hpp"
#include "util/threadsafequeue.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <queue>

#include <QAudioSink>
#include <QAudioFormat>
#include <QImage>
#include <QMainWindow>
#include <QMediaDevices>
#include <QWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent, const std::string& filePath);
    ~MainWindow() override;

    static constexpr int GAME_WIDTH = 256 * 3;
    static constexpr int GAME_HEIGHT = 240 * 3;
    static constexpr int DEBUG_WIDTH = 300;

public slots:
    void displayNewFrame(const QImage& image);
    void displayNewDebugFrame(const DebugWindowState& state);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    EmulatorThread* emulatorThread;
    GameWindow* gameWindow;
    DebugWindow* debugWindow;

    ControllerStatus controllerStatus;
    std::atomic<bool> resetFlag;
    std::atomic<bool> debugWindowEnabled;
    std::atomic<uint8_t> stepModeEnabled;
    std::atomic<bool> stepRequested;
    std::atomic<uint8_t> spritePallete;
    std::atomic<uint8_t> backgroundPallete;
    std::atomic<uint8_t> globalMuteFlag;

    void setControllerData(bool controller, Controller::Button button, bool value);

    const QAudioFormat audioFormat;
    QAudioSink* audioSink;
    AudioPlayer* audioPlayer;
    ThreadSafeQueue<float> queue;
    void updateAudioState();
    void resetAudioSink();

    static QAudioFormat defaultAudioFormat();
};

#endif // MAINWINDOW_HPP