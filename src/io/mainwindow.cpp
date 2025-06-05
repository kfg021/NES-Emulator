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

	localKeyInput = {};
	if (saveFilePathOption.has_value()) {
		localKeyInput.mostRecentSaveFilePath = QString::fromStdString(saveFilePathOption.value());
	}
	sharedKeyInput = localKeyInput;

	audioPlayer = new AudioPlayer(this, audioFormat, &audioSamples);
	audioSink = nullptr;
	createAudioSink();
	updateAudioState();

	debugWindowData = {};

	defaultAudioDevice = QMediaDevices::defaultAudioOutput();
	mediaDevices = new QMediaDevices(this);
	connect(mediaDevices, &QMediaDevices::audioOutputsChanged, this, &MainWindow::onAudioOutputsChanged);


	emulatorThread = new EmulatorThread(
		this,
		romFilePath,
		saveFilePathOption,
		sharedKeyInput,
		keyInputMutex,
		audioSamples
	);

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

void MainWindow::displayNewFrame(const QImage& image) {
	mainWindowData = image;
	update();
}

void MainWindow::displayNewDebugFrame(const DebugWindowState& state) {
	debugWindowData = state;

	// displayNewDebugFrame handles updates only when we are step-through mode
	if (localKeyInput.debugWindowEnabled && localKeyInput.paused) {
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
		localKeyInput.resetCount++;
	}

	// Pause button
	else if (event->key() == PAUSE_KEY) {
		localKeyInput.paused ^= 1;
		updateAudioState();
	}

	// Mute button
	else if (event->key() == MUTE_KEY) {
		localKeyInput.muted ^= 1;
		updateAudioState();
	}

	// Save states
	else if (event->key() == SAVE_KEY) {
		// Block the emulator thread until the user chooses a file or closes the file dialog
		{
			std::lock_guard<std::mutex> guard(keyInputMutex);
			localKeyInput.mostRecentSaveFilePath = QFileDialog::getSaveFileName(
				this, "Create save state", QDir::homePath(), "(*.sstate)"
			);
			localKeyInput.saveCount++;

			sharedKeyInput = localKeyInput;
		}

		// Already passed user input to emulator thread, so we can return
		return;
	}

	else if (event->key() == LOAD_KEY) {
		// Block the emulator thread until the user chooses a file or closes the file dialog
		{
			std::lock_guard<std::mutex> guard(keyInputMutex);
			localKeyInput.mostRecentSaveFilePath = QFileDialog::getOpenFileName(
				this, "Load save state", QDir::homePath(), "(*.sstate)"
			);
			localKeyInput.loadCount++;

			sharedKeyInput = localKeyInput;
		}

		// Already passed user input to emulator thread, so we can return
		return;
	}

	else if (event->key() == QUICK_LOAD_KEY) {
		localKeyInput.loadCount++;
	}

	// Debugging keys
	else if (event->key() == DEBUG_WINDOW_KEY) {
		if (localKeyInput.debugWindowEnabled) {
			localKeyInput.debugWindowEnabled = false;
			setFixedSize(GAME_WIDTH, GAME_HEIGHT);
		}
		else {
			localKeyInput.debugWindowEnabled = true;
			setFixedSize(TOTAL_WIDTH, GAME_HEIGHT);
		}
	}
	else if (localKeyInput.debugWindowEnabled) {
		if (event->key() == STEP_KEY) {
			if (localKeyInput.paused) {
				localKeyInput.stepCount++;
			}
		}
		else if (event->key() == BACKGROUND_PALLETE_KEY) {
			localKeyInput.backgroundPallete = (localKeyInput.backgroundPallete + 1) & 3;
		}
		else if (event->key() == SPRITE_PALLETE_KEY) {
			localKeyInput.spritePallete = (localKeyInput.spritePallete + 1) & 3;
		}
	}

	// Pass the user input to the main thread
	{
		std::lock_guard<std::mutex> guard(keyInputMutex);
		sharedKeyInput = localKeyInput;
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

	{
		std::lock_guard<std::mutex> guard(keyInputMutex);
		sharedKeyInput = localKeyInput;
	}
}

void MainWindow::setControllerData(bool controller, Controller::Button button, bool value) {
	uint8_t& controllerData = !controller ? localKeyInput.controller1ButtonMask : localKeyInput.controller1ButtonMask;
	uint8_t bitToChange = (1 << static_cast<int>(button));
	if (value) {
		controllerData |= bitToChange;
	}
	else {
		controllerData &= ~bitToChange;
	}
}


void MainWindow::updateAudioState() {
	bool muted = localKeyInput.muted || localKeyInput.paused;
	if (muted) {
		if (audioSink && audioSink->state() != QAudio::SuspendedState && audioSink->state() != QAudio::StoppedState) {
			audioSink->suspend();
		}
	}
	else {
		if (audioSink && audioSink->state() == QAudio::SuspendedState) {
			audioSink->resume();
		}
	}
}


void MainWindow::createAudioSink() {
	if (!audioSink) {
		defaultAudioDevice = QMediaDevices::defaultAudioOutput();
		audioSink = new QAudioSink(defaultAudioDevice, audioFormat, this);
		audioSink->start(audioPlayer);
	}
}

void MainWindow::onAudioOutputsChanged() {
	auto newDefaultAudioDevice = QMediaDevices::defaultAudioOutput();
	if (newDefaultAudioDevice != defaultAudioDevice) {
		defaultAudioDevice = newDefaultAudioDevice;

		if (audioSink) {
			audioSink->stop();
			delete audioSink;
			audioSink = nullptr;
		}

		createAudioSink();
	}
}

void MainWindow::paintEvent(QPaintEvent* /*event*/) {
	if (!mainWindowData.isNull()) {
		QPainter painter(this);
		const QPixmap pixmap = QPixmap::fromImage(mainWindowData);
		painter.drawPixmap(0, 0, MainWindow::GAME_WIDTH, MainWindow::GAME_HEIGHT, pixmap);
	}

	if (localKeyInput.debugWindowEnabled) {
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

	const std::array<uint32_t, 0x20>& palletes = debugWindowData.palleteRamColors;

	static constexpr int PALLETE_DISPLAY_SIZE = 7;
	static constexpr int NUM_INSTS = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW;

	if (debugWindowData.patternTables) {
		{
			// Display each of the 4 background color palletes
			painter.setBrush(Qt::white);
			painter.drawRect(START_X + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.backgroundPallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

			for (int i = 0; i < 4; i++) {
				QImage backgroundPallete(reinterpret_cast<const uint8_t*>(&palletes[i * 4]), 4, 1, QImage::Format::Format_ARGB32);
				const QPixmap palletePixmap = QPixmap::fromImage(backgroundPallete);
				painter.drawPixmap(START_X + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
			}

			QImage backgroundPattern(reinterpret_cast<const uint8_t*>(&debugWindowData.patternTables->backgroundPatternTable), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
			const QPixmap pixmap = QPixmap::fromImage(backgroundPattern);
			painter.drawPixmap(START_X, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap);
		}

		{
			// Display each of the 4 sprite color palletes
			static constexpr int START_TABLE_2 = TOTAL_WIDTH - X_OFFSET - PPU::PATTERN_TABLE_SIZE;

			painter.setBrush(Qt::white);
			painter.drawRect(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * debugWindowData.spritePallete - 2, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS) - 2, 4 * PALLETE_DISPLAY_SIZE + 4, 1 * PALLETE_DISPLAY_SIZE + 4);

			for (int i = 0; i < 4; i++) {
				QImage spritePallete(reinterpret_cast<const uint8_t*>(&palletes[0x10 + i * 4]), 4, 1, QImage::Format::Format_ARGB32);
				const QPixmap palletePixmap = QPixmap::fromImage(spritePallete);
				painter.drawPixmap(START_TABLE_2 + (PALLETE_DISPLAY_SIZE * 5) * i, START_Y + LETTER_HEIGHT * (9 + 2 * NUM_INSTS), 4 * PALLETE_DISPLAY_SIZE, 1 * PALLETE_DISPLAY_SIZE, palletePixmap);
			}

			QImage spritePattern(reinterpret_cast<const uint8_t*>(&debugWindowData.patternTables->spritePatternTable), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, QImage::Format::Format_ARGB32);
			const QPixmap pixmap = QPixmap::fromImage(spritePattern);
			painter.drawPixmap(START_TABLE_2, START_Y + LETTER_HEIGHT * (10 + 2 * NUM_INSTS), PPU::PATTERN_TABLE_SIZE, PPU::PATTERN_TABLE_SIZE, pixmap);
		}
	}
}