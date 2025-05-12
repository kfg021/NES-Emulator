#ifndef AUDIOPLAYER_HPP
#define AUDIOPLAYER_HPP

#include <cstdint>
#include <mutex>
#include <queue>

#include <QAudioFormat>
#include <QIODevice>
#include <QWidget>

class AudioPlayer : public QIODevice {
    Q_OBJECT

public:
    AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, bool muted);

    void addSample(float sample);
    void tryToMute();
    void tryToUnmute();

protected:
    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t maxSize) override;
    int64_t bytesAvailable() const override;

private:
    mutable std::mutex mtx;

    std::queue<float> audioSamples;
    QAudioFormat audioFormat;

    bool muted;
};

#endif // AUDIOPLAYER_HPP