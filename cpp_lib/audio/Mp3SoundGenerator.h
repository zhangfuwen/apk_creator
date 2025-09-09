#include <memory>

#include "ndk_utils/log.h"
#include "dr_mp3.h"
#include "TappableAudioSource.h"
#include "oboe/Definitions.h"
#include "oboe/Oboe.h"

class Mp3SoundGenerator : public TappableAudioSource {
    static constexpr size_t kSharedBufferSize = 1024*1024;

  public:
    static oboe::ResultWithValue<std::shared_ptr<Mp3SoundGenerator>> createFromFile(std::string filepath) {
        drmp3 mp3;
        if (!drmp3_init_file(&mp3, filepath.c_str(), NULL)) {
            LOGE("Failed to open MP3");
            return oboe::ResultWithValue<std::shared_ptr<Mp3SoundGenerator>>(oboe::Result::ErrorNull);
        }

        auto pcm_frame_count = drmp3_get_pcm_frame_count(&mp3);
        auto channels = mp3.channels;
        auto sample_rate = mp3.sampleRate;

        LOGI("MP3 file has %lu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
        auto ret = oboe::ResultWithValue(std::make_shared<Mp3SoundGenerator>(sample_rate, channels));
        ret.value()->mMp3 = mp3;
        return ret;
    }

    static oboe::ResultWithValue<std::shared_ptr<Mp3SoundGenerator>> createFromBuf(char* buf, int size) {
        drmp3 mp3;
        if (!drmp3_init_memory(&mp3, buf, size, nullptr)) {
            LOGE("Failed to open MP3");
            return oboe::ResultWithValue<std::shared_ptr<Mp3SoundGenerator>>(oboe::Result::ErrorNull);
        }

        auto pcm_frame_count = drmp3_get_pcm_frame_count(&mp3);
        auto channels = mp3.channels;
        auto sample_rate = mp3.sampleRate;

        LOGI("MP3 file has %lu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
        auto ret = oboe::ResultWithValue(std::make_shared<Mp3SoundGenerator>(sample_rate, channels));
        ret.value()->mMp3 = mp3;
        return ret;
    }

    int resetData(char* buf, int size) {
        LOGI("Reset MP3 data");
        std::lock_guard<std::mutex> lock(mMutex);
        if (!drmp3_init_memory(&mMp3, buf, size, nullptr)) {
            LOGE("Failed to open MP3");
            return -1;
        }
        auto pcm_frame_count = drmp3_get_pcm_frame_count(&mMp3);
        auto channels = mMp3.channels;
        auto sample_rate = mMp3.sampleRate;

        LOGI("MP3 file has %lu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
        mSampleRate = sample_rate;
        mChannelCount = channels;
        mTotalFrames = pcm_frame_count;
        return 0;
    }

    // Switch the tones on
    void tap(bool isOn) override { }

    void renderAudio(float* audioData, int32_t numFrames) override {
        LOGI("renderAudio numFrames %d", numFrames);
        std::lock_guard<std::mutex> lock(mMutex);
        std::fill_n(mBuffer.get(), kSharedBufferSize, 0);
        for (int i = 0; i < mChannelCount; ++i) {
            auto ret = drmp3_read_pcm_frames_f32(&mMp3, numFrames, mBuffer.get());
            if (ret <= 0) {
                LOGE("Decode failed");
                std::fill_n(mBuffer.get(), kSharedBufferSize, 0);
            }
            for (int j = 0; j < ret; ++j) {
                audioData[(j * mChannelCount) + i] = mBuffer[j];
            }
        }
    }

    Mp3SoundGenerator(int32_t sampleRate, int32_t channelCount)
        : TappableAudioSource(sampleRate, channelCount) { }

    ~Mp3SoundGenerator() { }

  private:
    drmp3                    mMp3;
    uint64_t                 mTotalFrames;
    std::unique_ptr<float[]> mBuffer = std::make_unique<float[]>(kSharedBufferSize);
    std::mutex               mMutex;
};
