#pragma once
#include "Oscillator.h"
#include "TappableAudioSource.h"

/**
 * Generates a fixed frequency tone for each channel.
 * Implements RenderableTap (sound source with toggle) which is required for AudioEngines.
 */
class SoundGenerator : public TappableAudioSource {
    static constexpr size_t kSharedBufferSize = 1024;
public:
    /**
     * Create a new SoundGenerator object.
     *
     * @param sampleRate - The output sample rate.
     * @param maxFrames - The maximum number of audio frames which will be rendered, this is used to
     * calculate this object's internal buffer size.
     * @param channelCount - The number of channels in the output, one tone will be created for each
     * channel, the output will be interlaced.
     *
     */
    SoundGenerator(int32_t sampleRate, int32_t channelCount);
    ~SoundGenerator() = default;

    SoundGenerator(SoundGenerator&& other) = default;
    SoundGenerator& operator= (SoundGenerator&& other) = default;

    // Switch the tones on
    void tap(bool isOn) override;

    void renderAudio(float *audioData, int32_t numFrames) override;

private:
    std::unique_ptr<Oscillator[]> mOscillators;
    std::unique_ptr<float[]> mBuffer = std::make_unique<float[]>(kSharedBufferSize);
};


