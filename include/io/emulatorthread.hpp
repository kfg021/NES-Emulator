#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "io/guitypes.hpp"
#include "io/savestate.hpp"
#include "io/threadsafeaudioqueue.hpp"

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
		const KeyboardInput& keyInput,
		ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples
	);
	~EmulatorThread() override;
	void run() override;

	void requestStop();
	void requestSoundReactivation();

signals:
	void soundReadySignal();
	void frameReadySignal(const QImage& display);
	void debugFrameReadySignal(const DebugWindowState& state);

private:
	static constexpr int FPS = 60;
	static constexpr int TARGET_FRAME_US = 1000000 / FPS;

	// 262 scanlines, 341 cycles per scanline, 3 PPU cycles per CPU cycle
	static constexpr int EXPECTED_CPU_CYCLES_PER_FRAME = (262 * 341) / 3;
	static constexpr int INSTRUCTIONS_PER_SECOND = EXPECTED_CPU_CYCLES_PER_FRAME * FPS;

	Bus bus;
	std::atomic<bool> isRunning;
	std::atomic<bool> soundActivated;

	// EmulatorThread is not responsible for managing this memory
	KeyboardInput keyInput;

	void runCycles();
	std::queue<uint16_t> recentPCs;
	std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> getInsts() const;

	ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples;

	int scaledAudioClock;

	// Save states
	SaveState saveState;
};

#endif // EMULATORTHREAD_HPP