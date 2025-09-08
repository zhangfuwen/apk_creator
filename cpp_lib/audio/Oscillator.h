#pragma once

#include <cstdint>
#include <atomic>
#include <math.h>
#include <memory>
#include "IRenderableAudio.h"

constexpr double kDefaultFrequency = 440.0;
constexpr int32_t kDefaultSampleRate = 48000;
constexpr double kPi = M_PI;
constexpr double kTwoPi = kPi * 2;

class Oscillator : public IRenderableAudio {

public:

    ~Oscillator() = default;

    void setWaveOn(bool isWaveOn) {
        mIsWaveOn.store(isWaveOn);
    };

    void setSampleRate(int32_t sampleRate) {
        mSampleRate = sampleRate;
        updatePhaseIncrement();
    };

    void setFrequency(double frequency) {
        mFrequency = frequency;
        updatePhaseIncrement();
    };

    inline void setAmplitude(float amplitude) {
        mAmplitude = amplitude;
    };

    // From IRenderableAudio
    void renderAudio(float *audioData, int32_t numFrames) override {

        if (mIsWaveOn){
            for (int i = 0; i < numFrames; ++i) {

                // Sine wave (sinf)
                //audioData[i] = sinf(mPhase) * mAmplitude;

                // Square wave
                if (mPhase <= kPi){
                    audioData[i] = -mAmplitude;
                } else {
                    audioData[i] = mAmplitude;
                }

                mPhase += mPhaseIncrement;
                if (mPhase > kTwoPi) mPhase -= kTwoPi;
            }
        } else {
            memset(audioData, 0, sizeof(float) * numFrames);
        }
    };

private:
    std::atomic<bool> mIsWaveOn { false };
    float mPhase = 0.0;
    std::atomic<float> mAmplitude { 0 };
    std::atomic<double> mPhaseIncrement { 0.0 };
    double mFrequency = kDefaultFrequency;
    int32_t mSampleRate = kDefaultSampleRate;

    void updatePhaseIncrement(){
        mPhaseIncrement.store((kTwoPi * mFrequency) / static_cast<double>(mSampleRate));
    };
};
