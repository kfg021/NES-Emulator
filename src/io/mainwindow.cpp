#include "io/mainwindow.hpp"

#include "core/controller.hpp"
#include "core/cpu.hpp"
#include "io/emulatorthread.hpp"

#include <QColor>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>

QAudioFormat MainWindow::defaultAudioFormat() {
	QAudioFormat audioFormat;
	audioFormat.setSampleRate(AUDIO_SAMPLE_RATE);
	audioFormat.setChannelCount(1); // Mono
	audioFormat.setSampleFormat(QAudioFormat::Float);
	return audioFormat;
}

MainWindow::MainWindow(QWidget* parent, const std::string& romFilePath, const std::optional<std::string>& saveFilePathOption)
	: QMainWindow(parent),
	audioFormat(defaultAudioFormat()) {
	setWindowTitle("NES Emulator");
	setFixedSize(GAME_WIDTH, GAME_HEIGHT);

	controllerStatus[0].store(0, std::memory_order_relaxed);
	controllerStatus[1].store(0, std::memory_order_relaxed);
	resetFlag.store(false, std::memory_order_relaxed);
	debugWindowEnabled.store(false, std::memory_order_relaxed);
	pauseFlag.store(false, std::memory_order_relaxed);
	stepRequested.store(false, std::memory_order_relaxed);
	spritePallete.store(0, std::memory_order_relaxed);
	backgroundPallete.store(0, std::memory_order_relaxed);
	globalMuteFlag.store(0, std::memory_order_relaxed);

	bool muted = globalMuteFlag.load(std::memory_order_relaxed) & 1;
	audioPlayer = new AudioPlayer(this, audioFormat, muted, &audioSamples);
	audioSink = nullptr;

	defaultAudioDevice = QMediaDevices::defaultAudioOutput();
	mediaDevices = new QMediaDevices(this);
	connect(mediaDevices, &QMediaDevices::audioOutputsChanged, this, &MainWindow::onDefaultAudioDeviceChanged);

	KeyboardInput keyInput = {
		&controllerStatus,
		&resetFlag,
		&pauseFlag,
		&debugWindowEnabled,
		&stepRequested,
		&spritePallete,
		&backgroundPallete,
		&globalMuteFlag,
		&saveRequested,
		&loadRequested,
		&saveFilePath,
	};
	emulatorThread = new EmulatorThread(this, romFilePath, saveFilePathOption, keyInput, &audioSamples);

	connect(emulatorThread, &EmulatorThread::soundReadySignal, this, &MainWindow::enableAudioSink, Qt::QueuedConnection);
	connect(emulatorThread, &EmulatorThread::frameReadySignal, this, &MainWindow::displayNewFrame, Qt::QueuedConnection);
	connect(emulatorThread, &EmulatorThread::debugFrameReadySignal, this, &MainWindow::displayNewDebugFrame, Qt::QueuedConnection);

	emulatorThread->start();
}

MainWindow::~MainWindow() {
	if (emulatorThread) {
		emulatorThread->requestStop();

		static constexpr int MAX_WAIT_MS = 1000;
		if (!emulatorThread->wait(MAX_WAIT_MS)) {
			// Thread still running, terminate it as a last resort
			emulatorThread->terminate();
			emulatorThread->wait();
		}
	}

	if (audioSink) {
		if (audioSink->state() != QAudio::StoppedState) {
			audioSink->stop();
		}
	}
}

void MainWindow::enableAudioSink() {
	createAudioSink();
}

void MainWindow::displayNewFrame(const QImage& image) {
	mainWindowData = image;
	update();
}

void MainWindow::displayNewDebugFrame(const DebugWindowState& state) {
	debugWindowData = state;

	// displayNewDebugFrame handles updates only when we are step-through mode
	if (debugWindowEnabled.load(std::memory_order_relaxed) && pauseFlag.load(std::memory_order_acquire)) {
		update();
	}
}

// TODO: Key inputs for second controller
void MainWindow::keyPressEvent(QKeyEvent* event) {
	// Game keys
	if (event->key() == UP_KEY) {
		setControllerData(0, Controller::Button::UP, 1);
	}
	else if (event->key() == DOWN_KEY) {
		setControllerData(0, Controller::Button::DOWN, 1);
	}
	else if (event->key() == LEFT_KEY) {
		setControllerData(0, Controller::Button::LEFT, 1);
	}
	else if (event->key() == RIGHT_KEY) {
		setControllerData(0, Controller::Button::RIGHT, 1);
	}
	else if (event->key() == SELECT_KEY) {
		setControllerData(0, Controller::Button::SELECT, 1);
	}
	else if (event->key() == START_KEY) {
		setControllerData(0, Controller::Button::START, 1);
	}
	else if (event->key() == B_KEY) {
		setControllerData(0, Controller::Button::B, 1);
	}
	else if (event->key() == A_KEY) {
		setControllerData(0, Controller::Button::A, 1);
	}

	// Reset button
	else if (event->key() == RESET_KEY) {
		resetFlag.store(true, std::memory_order_relaxed);
	}

	// Pause button
	else if (event->key() == PAUSE_KEY) {
		pauseFlag.fetch_xor(1, std::memory_order_relaxed);
		updateAudioState();
	}

	// Mute button
	else if (event->key() == MUTE_KEY) {
		globalMuteFlag.fetch_xor(1, std::memory_order_relaxed);
		updateAudioState();
	}

	// Save states
	else if (event->key() == SAVE_KEY) {
		audioSamples.erase();
		pauseFlag.store(1, std::memory_order_release);
		updateAudioState();

		saveFilePath = QFileDialog::getSaveFileName(
			this, "Create save state", QDir::homePath(), "(*.sstate)"
		);

		saveRequested.store(true, std::memory_order_release);
		emulatorThread->requestSoundReactivation();
	}

	else if (event->key() == LOAD_KEY) {
		audioSamples.erase();
		pauseFlag.store(1, std::memory_order_release);
		updateAudioState();

		saveFilePath = QFileDialog::getOpenFileName(
			this, "Load save state", QDir::homePath(), "(*.sstate)"
		);

		loadRequested.store(true, std::memory_order_release);
		emulatorThread->requestSoundReactivation();
	}

	else if (event->key() == QUICK_LOAD_KEY) {
		audioSamples.erase();
		pauseFlag.store(1, std::memory_order_release);
		updateAudioState();

		loadRequested.store(true, std::memory_order_release);
		emulatorThread->requestSoundReactivation();
	}

	// Debugging keys
	else if (event->key() == DEBUG_WINDOW_KEY) {
		toggleDebugMode();
	}
	else if (debugWindowEnabled.load(std::memory_order_relaxed)) {
		if (event->key() == STEP_KEY) {
			if (pauseFlag.load(std::memory_order_relaxed)) {
				stepRequested.store(true, std::memory_order_relaxed);
			}
		}
		else if (event->key() == BACKGROUND_PATTETE_KEY) {
			backgroundPallete.fetch_add(1, std::memory_order_relaxed);
		}
		else if (event->key() == BACKGROUND_PATTETE_KEY) {
			spritePallete.fetch_add(1, std::memory_order_relaxed);
		}
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
	// Game keys
	if (event->key() == UP_KEY) {
		setControllerData(0, Controller::Button::UP, 0);
	}
	else if (event->key() == DOWN_KEY) {
		setControllerData(0, Controller::Button::DOWN, 0);
	}
	else if (event->key() == LEFT_KEY) {
		setControllerData(0, Controller::Button::LEFT, 0);
	}
	else if (event->key() == RIGHT_KEY) {
		setControllerData(0, Controller::Button::RIGHT, 0);
	}
	else if (event->key() == SELECT_KEY) {
		setControllerData(0, Controller::Button::SELECT, 0);
	}
	else if (event->key() == START_KEY) {
		setControllerData(0, Controller::Button::START, 0);
	}
	else if (event->key() == B_KEY) {
		setControllerData(0, Controller::Button::B, 0);
	}
	else if (event->key() == A_KEY) {
		setControllerData(0, Controller::Button::A, 0);
	}
}

void MainWindow::setControllerData(bool controller, Controller::Button button, bool value) {
	uint8_t bitToChange = (1 << static_cast<int>(button));
	if (value) {
		controllerStatus[controller].fetch_or(bitToChange, std::memory_order_relaxed);
	}
	else {
		controllerStatus[controller].fetch_and(~bitToChange, std::memory_order_relaxed);
	}
}


void MainWindow::updateAudioState() {
	auto tryToMute = [&]() -> void {
		if (audioSink && audioSink->state() != QAudio::SuspendedState && audioSink->state() != QAudio::StoppedState) {
			audioSink->suspend();
		}
		audioPlayer->tryToMute();
	};

	auto tryToUnmute = [&]() -> void {
		audioPlayer->tryToUnmute();
		if (audioSink && audioSink->state() == QAudio::SuspendedState) {
			audioSink->resume();
		}
	};

	bool globalMute = globalMuteFlag.load(std::memory_order_acquire) & 1;
	bool paused = pauseFlag.load(std::memory_order_acquire) & 1;
	if (globalMute || paused) {
		tryToMute();
	}
	else {
		tryToUnmute();
	}
}

void MainWindow::createAudioSink() {
	if (!audioSink) {
		defaultAudioDevice = QMediaDevices::defaultAudioOutput();
		audioSink = new QAudioSink(defaultAudioDevice, audioFormat, this);
		audioSink->start(audioPlayer);
	}

	updateAudioState();
}

void MainWindow::onDefaultAudioDeviceChanged() {
	auto newDefaultAudioDevice = QMediaDevices::defaultAudioOutput();
	if (newDefaultAudioDevice != defaultAudioDevice) {
		defaultAudioDevice = newDefaultAudioDevice;

		if (audioSink) {
			bool unmuted = !(globalMuteFlag.load(std::memory_order_relaxed) & 1);
			// Mute sound so we don't miss any samples while we reset audio sink
			if (unmuted) {
				globalMuteFlag.store(true, std::memory_order_relaxed);
				updateAudioState();
			}

			audioSink->stop();
			delete audioSink;
			audioSink = nullptr;

			if (unmuted) {
				globalMuteFlag.store(false, std::memory_order_relaxed);
			}
		}

		emulatorThread->requestSoundReactivation();
	}
}

void MainWindow::toggleDebugMode() {
	bool debugMode = debugWindowEnabled.load(std::memory_order_relaxed);
	if (debugMode) {
		debugWindowEnabled.store(false, std::memory_order_relaxed);
		setFixedSize(GAME_WIDTH, GAME_HEIGHT);
	}
	else {
		debugWindowEnabled.store(true, std::memory_order_release);
		setFixedSize(TOTAL_WIDTH, GAME_HEIGHT);
	}
}

void MainWindow::paintEvent(QPaintEvent* /*event*/) {
	if (!mainWindowData.isNull()) {
		QPainter painter(this);
		const QPixmap pixmap = QPixmap::fromImage(mainWindowData);
		painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
	}

	if (debugWindowEnabled.load(std::memory_order_relaxed)) {
		renderDebugWindow();
	}
}

void MainWindow::renderDebugWindow() {
	static constexpr int X_OFFSET = 15;
	static constexpr int START_X = GAME_WIDTH + X_OFFSET;
	static constexpr int START_Y = 20;
	static constexpr int LETTER_HEIGHT = 20;

	QPainter painter(this);
	QColor defaultColor = painter.pen().color();
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPixelSize(LETTER_HEIGHT);
	painter.setFont(font);
	int letterWidth = QFontMetrics(font).horizontalAdvance("A");

	const QString flagStr = "FLAGS:     ";
	painter.drawText(START_X, START_Y, flagStr);

	QString flagNames = "NV-BDIZC";
	for (int i = 0; i < 8; i++) {
		uint8_t sr = debugWindowData.sr;
		uint8_t flag = 7 - i;
		if ((sr >> flag) & 1) {
			painter.setPen(Qt::green);
		}
		else {
			painter.setPen(Qt::red);
		}

		QString flagStatusStr(1, flagNames[i]);
		painter.drawText(START_X + (flagStr.size() + i) * letterWidth, START_Y, flagStatusStr);
	}

	std::string pcStr = "PC:        $" + toHexString16(debugWindowData.pc);
	std::string aStr = "A:         $" + toHexString8(debugWindowData.a);
	std::string xStr = "X:         $" + toHexString8(debugWindowData.x);
	std::string yStr = "Y:         $" + toHexString8(debugWindowData.y);
	std::string spStr = "SP:        $" + toHexString8(debugWindowData.sp);

	painter.setPen(defaultColor);
	painter.drawText(START_X, START_Y + LETTER_HEIGHT * 2, QString(pcStr.c_str()));
	painter.drawText(START_X, START_Y + LETTER_HEIGHT * 3, QString(aStr.c_str()));
	painter.drawText(START_X, START_Y + LETTER_HEIGHT * 4, QString(xStr.c_str()));
	painter.drawText(START_X, START_Y + LETTER_HEIGHT * 5, QString(yStr.c_str()));
	painter.drawText(START_X, START_Y + LETTER_HEIGHT * 6, QString(spStr.c_str()));

	for (int i = 0; i < DebugWindowState::NUM_INSTS_TOTAL; i++) {
		if (i == DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW) {
			painter.setPen(Qt::cyan);
		}
		else {
			painter.setPen(defaultColor);
		}

		painter.drawText(START_X, START_Y + LETTER_HEIGHT * (8 + i), debugWindowData.insts[i]);
	}

	const std::array<uint32_t, 0x20> palletes = debugWindowData.palleteRamColors;

	static constexpr int PALLETE_DISPLAY_SIZE = 7;
	static constexpr int NUM_INSTS = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW;

	{
		// Display each of the 4 background color palletes
		painter.setBrush(Qt::white);
		painter.drawRect(START_X + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.backgroundPallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

		for (int i = 0; i < 4; i++) {
			QImage image3(reinterpret_cast<const uint8_t*>(&palletes[i * 4]), 4, 1, QImage::Format::Format_ARGB32);
			const QPixmap palletePixmap = QPixmap::fromImage(image3);
			painter.drawPixmap(START_X + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
		}
		const PPU::PatternTable table1 = debugWindowData.table1;
		QImage image1(reinterpret_cast<const uint8_t*>(&table1), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
		const QPixmap pixmap1 = QPixmap::fromImage(image1);
		painter.drawPixmap(START_X, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap1);
	}

	{
		// Display each of the 4 sprite color palletes
		static constexpr int START_TABLE_2 = TOTAL_WIDTH - X_OFFSET - PPU::PATTERN_TABLE_SIZE;

		painter.setBrush(Qt::white);
		painter.drawRect(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.spritePallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

		for (int i = 0; i < 4; i++) {
			QImage image3(reinterpret_cast<const uint8_t*>(&palletes[0x10 + i * 4]), 4, 1, QImage::Format::Format_ARGB32);
			const QPixmap palletePixmap = QPixmap::fromImage(image3);
			painter.drawPixmap(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
		}

		const PPU::PatternTable table2 = debugWindowData.table2;
		QImage image2(reinterpret_cast<const uint8_t*>(&table2), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
		const QPixmap pixmap2 = QPixmap::fromImage(image2);
		painter.drawPixmap(START_TABLE_2, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap2);
	}
}