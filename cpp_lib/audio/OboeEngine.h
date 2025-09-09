#pragma once
#include <oboe/Oboe.h>

#include "SoundGenerator.h"
#include "Mp3SoundGenerator.h"
#include "LatencyTuningCallback.h"
#include "IRestartable.h"
#include "DefaultErrorCallback.h"

constexpr int32_t kBufferSizeAutomatic = 0;

class OboeEngine : public IRestartable {
  public:
    OboeEngine();

    virtual ~OboeEngine() = default;

    void tap(bool isDown);
    void playMp3(uint8_t* data, int size) {
         mMp3Data = data;
         mMp3Size = size;

        if (mMp3AudioSource == nullptr) {
            auto result = Mp3SoundGenerator::createFromBuf((char*)mMp3Data, mMp3Size);
            if (result.error() == oboe::Result::OK) {
               mMp3AudioSource = result.value();
            } else {
                LOGE("Failed to create Mp3SoundGenerator: %s", oboe::convertToText(result.error()));
            }
        } else {
           mMp3AudioSource->resetData((char*)mMp3Data, mMp3Size);
        }

    }

    /**
     * Open and start a stream.
     * @param deviceId the audio device id, can be obtained through an {@link AudioDeviceInfo} object
     * using Java/JNI.
     * @return error or OK
     */
    oboe::Result start(oboe::AudioApi audioApi, int deviceId, int channelCount);
    /* Start using current settings. */
    oboe::Result start();

    /**
     * Stop and close the stream.
     */
    oboe::Result stop();


    // From IRestartable
    void restart() override;

    void setBufferSizeInBursts(int32_t numBursts);

    /**
     * Calculate the current latency between writing a frame to the output stream and
     * the same frame being presented to the audio hardware.
     *
     * Here's how the calculation works:
     *
     * 1) Get the time a particular frame was presented to the audio hardware
     * @see AudioStream::getTimestamp
     * 2) From this extrapolate the time which the *next* audio frame written to the stream
     * will be presented
     * 3) Assume that the next audio frame is written at the current time
     * 4) currentLatency = nextFramePresentationTime - nextFrameWriteTime
     *
     * @return  Output Latency in Milliseconds
     */
    double getCurrentOutputLatencyMillis();

    bool isLatencyDetectionSupported();

    bool isAAudioRecommended();

  private:
    oboe::Result reopenStream();
    oboe::Result openPlaybackStream();

    std::shared_ptr<oboe::AudioStream>     mStream = nullptr;
    std::shared_ptr<LatencyTuningCallback> mLatencyCallback = nullptr;
    std::shared_ptr<DefaultErrorCallback>  mErrorCallback = nullptr;
    std::shared_ptr<SoundGenerator>        mAudioSource = nullptr;
    std::shared_ptr<Mp3SoundGenerator>     mMp3AudioSource = nullptr;
    bool                                   mIsLatencyDetectionSupported = false;

    uint8_t* mMp3Data = nullptr;
    int      mMp3Size = 0;

    int32_t        mDeviceId = oboe::Unspecified;
    int32_t        mChannelCount = oboe::Unspecified;
    oboe::AudioApi mAudioApi = oboe::AudioApi::Unspecified;
    std::mutex     mLock;
};
