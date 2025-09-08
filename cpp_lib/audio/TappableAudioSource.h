#pragma once
#include <stdint.h>

#include "IRenderableAudio.h"
#include "ITappable.h"

/**
 * This class renders Float audio, but can be tapped to control.
 * It also contains members for sample rate and channel count
 */
class TappableAudioSource : public IRenderableAudio, public ITappable {
public:
    TappableAudioSource(int32_t sampleRate, int32_t channelCount) :
    mSampleRate(sampleRate), mChannelCount(channelCount) { }

    int32_t mSampleRate;
    int32_t mChannelCount;
};
