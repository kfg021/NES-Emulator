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

	localKeyInput = {};
	updateDebugWindowState();

	if (saveFilePathOption.has_value()) {
		localKeyInput.mostRecentSaveFilePath = QString::fromStdString(saveFilePathOption.value());
	}
	sharedKeyInput = localKeyInput;

	audioPlayer = new AudioPlayer(this, audioFormat, audioSamples);
	audioSink = nullptr;

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
	connect(emulatorThread, &EmulatorThread::soundReadySignal, this, &MainWindow::createAudioSink, Qt::QueuedConnection);

	emulatorThread->start();
}

MainWindow::~MainWindow() {
	if (audioSink) {
		if (audioSink->state() != QAudio::StoppedState) {
			audioSink->stop();
		}
	}

	if (emulatorThread) {
		emulatorThread->requestStop();

		static constexpr int MAX_WAIT_MS = 1000;
		if (!emulatorThread->wait(MAX_WAIT_MS)) {
			// Thread still running, terminate it as a last resort
			emulatorThread->terminate();
			emulatorThread->wait();
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
	switch (event->key()) {
		case UP_KEY:
			setControllerData(0, Controller::Button::UP, 1);
			break;
		case DOWN_KEY:
			setControllerData(0, Controller::Button::DOWN, 1);
			break;
		case LEFT_KEY:
			setControllerData(0, Controller::Button::LEFT, 1);
			break;
		case RIGHT_KEY:
			setControllerData(0, Controller::Button::RIGHT, 1);
			break;
		case SELECT_KEY:
			setControllerData(0, Controller::Button::SELECT, 1);
			break;
		case START_KEY:
			setControllerData(0, Controller::Button::START, 1);
			break;
		case B_KEY:
			setControllerData(0, Controller::Button::B, 1);
			break;
		case A_KEY:
			setControllerData(0, Controller::Button::A, 1);
			break;
		case RESET_KEY:
			localKeyInput.resetCount++;
			break;
		case PAUSE_KEY:
			localKeyInput.paused ^= 1;
			updateAudioState();
			break;
		case MUTE_KEY:
			localKeyInput.muted ^= 1;
			updateAudioState();
			break;
		case SAVE_KEY:
			// Block the emulator thread until the user chooses a file
			{
				std::lock_guard<std::mutex> guard(keyInputMutex);
				bool wasPaused = localKeyInput.paused;
				if (!wasPaused) {
					localKeyInput.paused = true;
					updateAudioState();
				}

				QString file = QFileDialog::getSaveFileName(
					this, "Create save state", QDir::homePath(), "(*.sstate)"
				);
				if (!file.isEmpty()) {
					localKeyInput.mostRecentSaveFilePath = file;
					localKeyInput.saveCount++;
				}

				if (!wasPaused) {
					localKeyInput.paused = false;
					updateAudioState();
				}

				sharedKeyInput = localKeyInput;
			}

			// Already passed user input to emulator thread, so we can return
			return;
		case LOAD_KEY:
			// Block the emulator thread until the user chooses a file
			{
				std::lock_guard<std::mutex> guard(keyInputMutex);

				bool wasPaused = localKeyInput.paused;
				if (!wasPaused) {
					localKeyInput.paused = true;
					updateAudioState();
				}

				QString file = QFileDialog::getOpenFileName(
					this, "Load save state", QDir::homePath(), "(*.sstate)"
				);
				if (!file.isEmpty()) {
					localKeyInput.mostRecentSaveFilePath = file;
					localKeyInput.loadCount++;
				}

				if (!wasPaused) {
					localKeyInput.paused = false;
					updateAudioState();
				}

				sharedKeyInput = localKeyInput;
			}

			// Already passed user input to emulator thread, so we can return
			return;
		case QUICK_LOAD_KEY:
			localKeyInput.loadCount++;
			break;
		case DEBUG_WINDOW_KEY:
			localKeyInput.debugWindowEnabled ^= 1;
			updateDebugWindowState();
			break;
		case STEP_KEY:
			if (localKeyInput.debugWindowEnabled && localKeyInput.paused) {
				localKeyInput.stepCount++;
			}
			break;
		case BACKGROUND_PALLETE_KEY:
			if (localKeyInput.debugWindowEnabled) {
				localKeyInput.backgroundPallete = (localKeyInput.backgroundPallete + 1) & 3;
			}
			break;
		case SPRITE_PALLETE_KEY:
			if (localKeyInput.debugWindowEnabled) {
				localKeyInput.spritePallete = (localKeyInput.spritePallete + 1) & 3;
			}
			break;
		default:
			break;
	}

	{
		std::lock_guard<std::mutex> guard(keyInputMutex);
		sharedKeyInput = localKeyInput;
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
	switch (event->key()) {
		case UP_KEY:
			setControllerData(0, Controller::Button::UP, 0);
			break;
		case DOWN_KEY:
			setControllerData(0, Controller::Button::DOWN, 0);
			break;
		case LEFT_KEY:
			setControllerData(0, Controller::Button::LEFT, 0);
			break;
		case RIGHT_KEY:
			setControllerData(0, Controller::Button::RIGHT, 0);
			break;
		case SELECT_KEY:
			setControllerData(0, Controller::Button::SELECT, 0);
			break;
		case START_KEY:
			setControllerData(0, Controller::Button::START, 0);
			break;
		case B_KEY:
			setControllerData(0, Controller::Button::B, 0);
			break;
		case A_KEY:
			setControllerData(0, Controller::Button::A, 0);
			break;
		default:
			break;
	}

	{
		std::lock_guard<std::mutex> guard(keyInputMutex);
		sharedKeyInput = localKeyInput;
	}
}

void MainWindow::setControllerData(bool controller, Controller::Button button, bool value) {
	uint8_t& controllerData = !controller ? localKeyInput.controller1ButtonMask : localKeyInput.controller2ButtonMask;
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
			audioSamples.erase();
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

		updateAudioState();
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

void MainWindow::updateDebugWindowState() {
	if (localKeyInput.debugWindowEnabled) {
		setFixedSize(TOTAL_WIDTH, GAME_HEIGHT);
	}
	else {
		setFixedSize(GAME_WIDTH, GAME_HEIGHT);
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