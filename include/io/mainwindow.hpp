#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "core/bus.hpp"
#include "io/audioplayer.hpp"
#include "io/emulatorthread.hpp"
#include "io/guitypes.hpp"
#include "io/threadsafeaudioqueue.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <queue>

#include <QAudioSink>
#include <QAudioFormat>
#include <QImage>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMediaDevices>
#include <QString>
#include <QWidget>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget* parent, const std::string& romFilePath, const std::optional<std::string>& saveFilePathOption);
	~MainWindow() override;

	static constexpr int GAME_WIDTH = 256 * 3;
	static constexpr int GAME_HEIGHT = 240 * 3;
	static constexpr int DEBUG_WIDTH = 300;
	static constexpr int TOTAL_WIDTH = GAME_WIDTH + DEBUG_WIDTH;

public slots:
	void displayNewFrame(const QImage& image);
	void displayNewDebugFrame(const DebugWindowState& state);

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	EmulatorThread* emulatorThread;

	KeyboardInput localKeyInput;
	KeyboardInput sharedKeyInput;
	std::mutex keyInputMutex;

	void setControllerData(bool controller, Controller::Button button, bool value);

	// Rendering
	QImage mainWindowData;
	DebugWindowState debugWindowData;
	void renderDebugWindow();
	void updateDebugWindowState();

	// Audio
	const QAudioFormat audioFormat;
	QMediaDevices* mediaDevices;
	QAudioDevice defaultAudioDevice;
	QAudioSink* audioSink;
	AudioPlayer* audioPlayer;
	ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY> audioSamples;
	void updateAudioState();
	void createAudioSink();
	void onAudioOutputsChanged();
	static QAudioFormat defaultAudioFormat();

	// NES controller controls
	static constexpr Qt::Key UP_KEY = Qt::Key_Up;
	static constexpr Qt::Key DOWN_KEY = Qt::Key_Down;
	static constexpr Qt::Key LEFT_KEY = Qt::Key_Left;
	static constexpr Qt::Key RIGHT_KEY = Qt::Key_Right;
	static constexpr Qt::Key SELECT_KEY = Qt::Key_Shift;
	static constexpr Qt::Key START_KEY = Qt::Key_Return;
	static constexpr Qt::Key B_KEY = Qt::Key_Z;
	static constexpr Qt::Key A_KEY = Qt::Key_X;

	// System controls
	static constexpr Qt::Key RESET_KEY = Qt::Key_R;
	static constexpr Qt::Key PAUSE_KEY = Qt::Key_C;
	static constexpr Qt::Key MUTE_KEY = Qt::Key_M;

	// Debug controls
	static constexpr Qt::Key DEBUG_WINDOW_KEY = Qt::Key_D;
	static constexpr Qt::Key STEP_KEY = Qt::Key_Space;
	static constexpr Qt::Key BACKGROUND_PALLETE_KEY = Qt::Key_O;
	static constexpr Qt::Key SPRITE_PALLETE_KEY = Qt::Key_P;

	// Save state controls
	static constexpr Qt::Key SAVE_KEY = Qt::Key_S;
	static constexpr Qt::Key LOAD_KEY = Qt::Key_L;
	static constexpr Qt::Key QUICK_LOAD_KEY = Qt::Key_K;
};

#endif // MAINWINDOW_HPP