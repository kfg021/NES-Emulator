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

	int desiredNextFrameUs = TARGET_FRAME_US;
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();

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

			audioSamples.erase();
			scaledAudioClock = 0;

			std::queue<uint16_t> empty;
			std::swap(recentPCs, empty);

			desiredNextFrameUs = (elapsedTimer.nsecsElapsed() / 1000) + TARGET_FRAME_US;
		}

		// Check load/save
		uint8_t numLoads = localKeyInput.loadCount - lastLoadCount;
		lastLoadCount = localKeyInput.loadCount;
		if (numLoads >= 1) { // Only perform a max of one load each frame
			audioSamples.erase();

			SaveState::LoadStatus saveStatus = saveState.loadSaveState(localKeyInput.mostRecentSaveFilePath);
			qDebug().noquote() << saveStatus.message;

			std::queue<uint16_t> empty;
			std::swap(recentPCs, empty);
		}


		uint8_t numSaves = localKeyInput.saveCount - lastSaveCount;
		lastSaveCount = localKeyInput.saveCount;
		if (numSaves >= 1) { // Only perform a max of one save each frame
			audioSamples.erase();

			SaveState::CreateStatus saveStatus = saveState.createSaveState(localKeyInput.mostRecentSaveFilePath);
			qDebug().noquote() << saveStatus.message;
		}

		// Handle controller input
		bus.setController(0, localKeyInput.controller1ButtonMask);
		bus.setController(1, localKeyInput.controller2ButtonMask);

		runCycles();

		if (!isRunning.load()) break;

		if (bus.ppu->frameReadyFlag) {
			const PPU::Display& display = *(bus.ppu->finishedDisplay);
			QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
			emit frameReadySignal(image.copy());
			bus.ppu->frameReadyFlag = false;
		}

		if (localKeyInput.debugWindowEnabled) {
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

		int currentTimeUs = elapsedTimer.nsecsElapsed() / 1000;
		int sleepTimeUs = desiredNextFrameUs - currentTimeUs;

		// Small sleep times are generally unreliable
		static constexpr int MIN_SLEEP_TIME_US = 1000;
		if (sleepTimeUs > MIN_SLEEP_TIME_US) {
			QThread::usleep(sleepTimeUs);
		}
		else if (sleepTimeUs > 0) {
			QThread::yieldCurrentThread();
		}
		else {
			// We missed the deadline for this frame, so reset the frame deadline.
			desiredNextFrameUs = currentTimeUs;

			QThread::yieldCurrentThread();
		}

		desiredNextFrameUs += TARGET_FRAME_US;
	}
}

void EmulatorThread::runCycles() {
	int cycles = 0;

	// Check steps
	uint8_t numSteps = localKeyInput.stepCount - lastStepCount;
	lastStepCount = localKeyInput.stepCount;

	auto executeCycle = [&](bool debugEnabled, bool audioEnabled) -> bool {
		uint16_t currentPC = bus.cpu->getPC();

		bus.executeCycle();
		cycles++;

		uint16_t nextPC = bus.cpu->getPC();

		if (audioEnabled) {
			while (scaledAudioClock >= INSTRUCTIONS_PER_SECOND) {
				float sample = bus.apu->getAudioSample();
				audioSamples.push(sample);

				scaledAudioClock -= INSTRUCTIONS_PER_SECOND;
			}
			scaledAudioClock += AUDIO_SAMPLE_RATE;
		}
		else {
			scaledAudioClock = 0;
		}

		if (nextPC != currentPC) {
			if (debugEnabled) {
				if (recentPCs.size() == DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW) {
					recentPCs.pop();
				}
				recentPCs.push(currentPC);
			}
			return true;
		}
		return false;
	};

	auto loopCondition = [&]() -> bool {
		static constexpr int UPPER_LIMIT_CYCLES = EXPECTED_CPU_CYCLES_PER_FRAME * 2;
		return isRunning.load() && !bus.ppu->frameReadyFlag && cycles < UPPER_LIMIT_CYCLES;
	};

	if (!localKeyInput.paused) {
		while (loopCondition()) {
			executeCycle(localKeyInput.debugWindowEnabled, !localKeyInput.muted);
		}
	}
	else {
		if (localKeyInput.paused) {
			for (int i = 0; i < numSteps; i++) {
				while (loopCondition()) {
					bool isNewInstruction = executeCycle(true, false);
					if (isNewInstruction) break;
				}
			}
		}
	}
}

std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> EmulatorThread::getInsts() const {
	std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> insts;
	std::queue<uint16_t> recentPCsCopy = recentPCs;

	auto toString = [&](uint16_t addr) {
		std::string text = "$" + toHexString16(addr) + ": " + bus.cpu->toString(addr);
		return QString(text.c_str());
	};

	// Last x PCs
	int start = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW - static_cast<int>(recentPCsCopy.size());
	for (int i = start; i < DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW; i++) {
		insts[i] = toString(recentPCsCopy.front());
		recentPCsCopy.pop();
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