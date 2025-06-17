#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "io/iotypes.hpp"
#include "io/savestate.hpp"

#include <array>
#include <optional>
#include <queue>

#include <QImage>
#include <QObject>
#include <QString>
#include <QThread>

class EmulatorThread : public QThread {
	Q_OBJECT
public:
	EmulatorThread(
		QObject* parent,
		const std::string& romFilePath,
		const std::optional<std::string>& saveFilePath,
		const KeyboardInput& sharedKeyInput,
		std::mutex& keyInputMutex,
		AudioQueue& audioSamples
	);
	~EmulatorThread() override;
	void run() override;

	void requestStop();

signals:
	void frameReadySignal(const QImage& display);
	void debugFrameReadySignal(const DebugWindowState& state);
	void soundReadySignal();

private:
	static constexpr int FPS = 60;
	static constexpr int TARGET_FRAME_NS = static_cast<int>(1e9) / FPS;

	// 262 scanlines, 341 cycles per scanline, 3 PPU cycles per CPU cycle
	static constexpr int EXPECTED_CPU_CYCLES_PER_FRAME = (262 * 341) / 3;
	static constexpr int INSTRUCTIONS_PER_SECOND = EXPECTED_CPU_CYCLES_PER_FRAME * FPS;

	// Audio pacing
	static constexpr size_t AUDIO_QUEUE_TARGET_FILL_SAMPLES = AUDIO_SAMPLE_RATE / 20; // 50ms audio latency

	Bus bus;
	std::atomic<bool> isRunning;
	bool soundReady;

	KeyboardInput localKeyInput;
	const KeyboardInput& sharedKeyInput;
	std::mutex& keyInputMutex;

	uint8_t lastResetCount;
	uint8_t lastStepCount;
	uint8_t lastFrameStepCount;
	uint8_t lastSaveCount;
	uint8_t lastLoadCount;
	bool debugWindowOpenLastFrame;

	bool executeCycle();
	void runUntilFrameReady();
	void runSteps(uint8_t numSteps);

	CircularBuffer<uint16_t, DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW> recentPCs;
	std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> getInsts() const;

	AudioQueue& audioSamples;

	int scaledAudioClock;

	// Save states
	SaveState saveState;
};

#endif // EMULATORTHREAD_HPP