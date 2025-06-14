#include "io/emulatorthread.hpp"
#include <cstdint>

#include <QDebug>
#include <QElapsedTimer>
#include <QString>

EmulatorThread::EmulatorThread(
	QObject* parent,
	const std::string& romFilePath,
	const std::optional<std::string>& saveFilePathOption,
	const KeyboardInput& sharedKeyInput,
	std::mutex& keyInputMutex,
	ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>& audioSamples
) :
	QThread(parent),
	sharedKeyInput(sharedKeyInput),
	keyInputMutex(keyInputMutex),
	audioSamples(audioSamples),
	saveState(bus, QString::fromStdString(romFilePath)) {
	isRunning.store(false);

	Cartridge::Status status = bus.tryInitDevices(romFilePath);
	if (status.code != Cartridge::Code::SUCCESS) {
		qFatal("%s", status.message.c_str());
	}

	if (saveFilePathOption.has_value()) {
		QString saveFilePath = QString::fromStdString(saveFilePathOption.value());
		SaveState::LoadStatus saveStatus = saveState.loadSaveState(saveFilePath);
		qDebug().noquote() << saveStatus.message;
	}

	scaledAudioClock = 0;
	soundReady = false;

	localKeyInput = {};
	lastResetCount = 0;
	lastStepCount = 0;
	lastSaveCount = 0;
	lastLoadCount = 0;

	qRegisterMetaType<DebugWindowState>("DebugWindowState");
}

EmulatorThread::~EmulatorThread() {
	isRunning.store(false);
	wait();
}

void EmulatorThread::requestStop() {
	isRunning.store(false);
}

void EmulatorThread::run() {
	isRunning.store(true);

	QElapsedTimer framePacingTimer;
	framePacingTimer.start();
	int64_t nextFrameTargetNs = framePacingTimer.nsecsElapsed() + TARGET_FRAME_NS;

	while (isRunning.load()) {
		// Load keyboard input
		{
			std::lock_guard<std::mutex> guard(keyInputMutex);
			localKeyInput = sharedKeyInput;
		}

		// Check reset
		uint8_t numResets = localKeyInput.resetCount - lastResetCount;
		lastResetCount = localKeyInput.resetCount;
		if (numResets >= 1) { // Only perform a max of one reset each frame
			bus.reset();

			recentPCs.erase();

			nextFrameTargetNs = framePacingTimer.nsecsElapsed() + TARGET_FRAME_NS;
		}

		// Check load/save
		uint8_t numLoads = localKeyInput.loadCount - lastLoadCount;
		lastLoadCount = localKeyInput.loadCount;
		if (numLoads >= 1) { // Only perform a max of one load each frame
			SaveState::LoadStatus saveStatus = saveState.loadSaveState(localKeyInput.mostRecentSaveFilePath);
			qDebug().noquote() << saveStatus.message;

			if (saveStatus.code == SaveState::LoadStatus::Code::SUCCESS) {
				recentPCs.erase();
			}
		}

		uint8_t numSaves = localKeyInput.saveCount - lastSaveCount;
		lastSaveCount = localKeyInput.saveCount;
		if (numSaves >= 1) { // Only perform a max of one save each frame
			SaveState::CreateStatus saveStatus = saveState.createSaveState(localKeyInput.mostRecentSaveFilePath);
			qDebug().noquote() << saveStatus.message;
		}

		// Handle controller input
		bus.setController(0, localKeyInput.controller1ButtonMask);
		bus.setController(1, localKeyInput.controller2ButtonMask);

		// Check audio
		bool muted = localKeyInput.muted || localKeyInput.paused;
		if (muted) scaledAudioClock = 0;

		// Check steps
		uint8_t numSteps = localKeyInput.stepCount - lastStepCount;
		lastStepCount = localKeyInput.stepCount;

		// Run emulator
		bool shouldOutputGameFrame;
		bool shouldOutputDebugFrame;
		if (!localKeyInput.paused) {
			runUntilFrameReady();
			shouldOutputGameFrame = true;
			shouldOutputDebugFrame = localKeyInput.debugWindowEnabled;
		}
		else if (numSteps) {
			runSteps(numSteps);
			shouldOutputGameFrame = bus.ppu->frameReadyFlag;
			shouldOutputDebugFrame = true;
		}
		else {
			shouldOutputGameFrame = false;
			shouldOutputDebugFrame = false;
		}

		if (!isRunning.load()) break;

		// Output frames
		if (shouldOutputGameFrame) {
			const PPU::Display& display = *(bus.ppu->finishedDisplay);
			QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format_ARGB32_Premultiplied);
			emit frameReadySignal(image.copy());
			bus.ppu->frameReadyFlag = false;
		}

		if (shouldOutputDebugFrame) {
			std::unique_ptr<PPU::PatternTables> patternTablesTemp = bus.ppu->getPatternTables(localKeyInput.backgroundPallete, localKeyInput.spritePallete);
			std::shared_ptr<PPU::PatternTables> patternTables(std::move(patternTablesTemp));

			DebugWindowState state = {
				bus.cpu->getPC(),
				bus.cpu->getA(),
				bus.cpu->getX(),
				bus.cpu->getY(),
				bus.cpu->getSP(),
				bus.cpu->getSR(),
				localKeyInput.backgroundPallete,
				localKeyInput.spritePallete,
				bus.ppu->getPalleteRamColors(),
				patternTables,
				getInsts()
			};

			emit debugFrameReadySignal(state);
		}

		// Calculate extra sleep time needed due to audio overflow
		// TODO: Also handle audio underflow
		if (!localKeyInput.muted) {
			size_t currentAudioQueueSize = audioSamples.size();
			if (currentAudioQueueSize > AUDIO_QUEUE_UPPER_THRESHOLD_SAMPLES) {
				// We have too much audio buffered, which means we are generating frames too quickly.
				// Let audio catch up by adding a small delay to the next video frame target
				int64_t excessAudioNs = ((currentAudioQueueSize - AUDIO_QUEUE_TARGET_FILL_SAMPLES) * static_cast<int64_t>(1e9)) / AUDIO_SAMPLE_RATE;
				if (excessAudioNs > static_cast<int64_t>(1e6)) { // 1ms
					// Delay next frame target by half of the excess
					int64_t delayNs = excessAudioNs / 2;
					nextFrameTargetNs += delayNs;
				}
			}
		}

		// Calculate total required sleep time
		int64_t currentTimeNs = framePacingTimer.nsecsElapsed();
		int64_t sleepTimeNs = nextFrameTargetNs - currentTimeNs;

		static constexpr int64_t MIN_SLEEP_TIME_NS = static_cast<int64_t>(1e6); // 1ms

		if (sleepTimeNs > MIN_SLEEP_TIME_NS) {
			QThread::usleep(sleepTimeNs / 1000LL);
		}
		else if (sleepTimeNs > 0) {
			QThread::yieldCurrentThread();
		}
		else {
			// We missed the deadline for this frame, so reset the frame deadline.
			nextFrameTargetNs = currentTimeNs;

			QThread::yieldCurrentThread();
		}

		nextFrameTargetNs += TARGET_FRAME_NS;
	}
}

bool EmulatorThread::executeCycle(bool debugEnabled, bool audioEnabled) {
	uint16_t currentPC = bus.cpu->getPC();

	bus.executeCycle();

	uint16_t nextPC = bus.cpu->getPC();

	if (audioEnabled) {
		while (scaledAudioClock >= INSTRUCTIONS_PER_SECOND) {
			audioSamples.forcePush(bus.apu->getAudioSample());
			if (!soundReady) {
				soundReady = true;
				emit soundReadySignal();
			}

			scaledAudioClock -= INSTRUCTIONS_PER_SECOND;
		}
		scaledAudioClock += AUDIO_SAMPLE_RATE;
	}

	if (nextPC != currentPC) {
		if (debugEnabled) {
			recentPCs.forcePush(currentPC);
		}
		return true;
	}
	return false;
}

void EmulatorThread::runUntilFrameReady() {
	int cycles = 0;
	static constexpr int CYCLE_LIMIT = EXPECTED_CPU_CYCLES_PER_FRAME + 5;

	while (!bus.ppu->frameReadyFlag && cycles < CYCLE_LIMIT) {
		executeCycle(localKeyInput.debugWindowEnabled, !localKeyInput.muted);
		cycles++;
	}
}

void EmulatorThread::runSteps(uint8_t numSteps) {
	for (int i = 0; i < numSteps; i++) {
		int cycles = 0;
		static constexpr int CYCLE_LIMIT = 100000;

		bool isNewInstruction = false;
		while (!isNewInstruction && cycles < CYCLE_LIMIT) {
			isNewInstruction = executeCycle(true, false);
			cycles++;
		}
	}
}

std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> EmulatorThread::getInsts() const {
	std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> insts;
	auto recentPCsCopy = recentPCs;

	auto toString = [&](uint16_t addr) {
		std::string text = "$" + toHexString16(addr) + ": " + bus.cpu->toString(addr);
		return QString(text.c_str());
	};

	// Last x PCs
	int start = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW - static_cast<int>(recentPCsCopy.size());
	for (int i = start; i < DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW; i++) {
		insts[i] = toString(recentPCsCopy.pop());
	}

	// Current PC and upcoming x PCs
	uint16_t lastPC = bus.cpu->getPC();
	for (int i = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW; i < DebugWindowState::NUM_INSTS_TOTAL; i++) {
		insts[i] = toString(lastPC);
		const CPU::Opcode& op = bus.cpu->getOpcode(lastPC);
		lastPC += op.addressingMode.instructionSize;
	}

	return insts;
}