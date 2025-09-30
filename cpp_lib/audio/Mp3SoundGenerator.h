#include <memory>

#include "ndk_utils/log.h"
#include "dr_mp3.h"
#include "TappableAudioSource.h"
#include "oboe/Definitions.h"
#include "oboe/Oboe.h"

#include <vector>
#include <cmath>
#include <stdexcept>

/**
 * Upsample PCM audio data using linear interpolation
 * 
 * @param input Input PCM samples (interleaved if multi-channel)
 * @param input_sample_rate Original sample rate (Hz)
 * @param output_sample_rate Target sample rate (Hz)
 * @param channels Number of audio channels (1 = mono, 2 = stereo, etc.)
 * @return Upsampled PCM data
 */
inline std::vector<float> upsample_pcm(const std::vector<float>& input, 
                                int input_sample_rate, 
                                int output_sample_rate, 
                                int channels = 1) {
    if (input_sample_rate <= 0 || output_sample_rate <= 0) {
        throw std::invalid_argument("Sample rates must be positive");
    }
    if (channels <= 0) {
        throw std::invalid_argument("Channels must be positive");
    }
    if (input.size() % channels != 0) {
        throw std::invalid_argument("Input size must be divisible by number of channels");
    }
    if (output_sample_rate <= input_sample_rate) {
        throw std::invalid_argument("Output sample rate must be higher than input for upsampling");
    }

    // Calculate upsampling ratio
    double ratio = static_cast<double>(output_sample_rate) / input_sample_rate;
    size_t input_frames = input.size() / channels;
    size_t output_frames = static_cast<size_t>(std::ceil(input_frames * ratio));
    
    std::vector<float> output(output_frames * channels);
    
    // Process each channel separately
    for (int ch = 0; ch < channels; ++ch) {
        for (size_t out_frame = 0; out_frame < output_frames; ++out_frame) {
            // Calculate corresponding position in input
            double input_pos = out_frame / ratio;
            
            // Get the two nearest input samples
            size_t input_frame_low = static_cast<size_t>(std::floor(input_pos));
            size_t input_frame_high = input_frame_low + 1;
            
            // Handle boundary conditions
            if (input_frame_low >= input_frames) {
                // Beyond end - use last sample
                output[out_frame * channels + ch] = input[(input_frames - 1) * channels + ch];
            } else if (input_frame_high >= input_frames) {
                // At the last sample - no interpolation needed
                output[out_frame * channels + ch] = input[input_frame_low * channels + ch];
            } else {
                // Linear interpolation
                float sample_low = input[input_frame_low * channels + ch];
                float sample_high = input[input_frame_high * channels + ch];
                float fraction = static_cast<float>(input_pos - input_frame_low);
                output[out_frame * channels + ch] = sample_low + fraction * (sample_high - sample_low);
            }
        }
    }
    
    return output;
}

// Helper function for integer PCM data (16-bit)
inline std::vector<int16_t> upsample_pcm_int16(const std::vector<int16_t>& input,
                                        int input_sample_rate,
                                        int output_sample_rate,
                                        int channels = 1) {
    // Convert to float
    std::vector<float> input_float(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        input_float[i] = static_cast<float>(input[i]) / 32768.0f;
    }
    
    // Upsample
    std::vector<float> output_float = upsample_pcm(input_float, input_sample_rate, 
                                                   output_sample_rate, channels);
    
    // Convert back to int16_t
    std::vector<int16_t> output(output_float.size());
    for (size_t i = 0; i < output_float.size(); ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, output_float[i]));
        output[i] = static_cast<int16_t>(clamped * 32767.0f);
    }
    
    return output;
}

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

        LOGI("MP3 file has %llu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
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

        LOGI("MP3 file has %llu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
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

        LOGI("MP3 file has %llu frames, %d ch, %d Hz", pcm_frame_count, channels, sample_rate);
        mSampleRate = sample_rate;
        mChannelCount = channels;
        mTotalFrames = pcm_frame_count;
        return 0;
    }

    // Switch the tones on
    void tap(bool isOn) override { (void)isOn; }

    void renderAudio(float* audioData, int32_t numFrames) override {
        LOGV("renderAudio numFrames %d", numFrames);
        std::lock_guard<std::mutex> lock(mMutex);
        if (mDeviceSampleRate == mSampleRate) {
            std::fill_n(mBuffer.get(), kSharedBufferSize, 0);
            auto ret = drmp3_read_pcm_frames_f32(&mMp3, numFrames, mBuffer.get());
            if (ret <= 0) {
                LOGV("Decode failed");
                std::fill_n(mBuffer.get(), kSharedBufferSize, 0);
            }
            // No need to resample
            for (int j = 0; j < (int)numFrames; ++j) {
                for (int i = 0; i < mDeviceChannelCount; ++i) {
                    if (mChannelCount == 2) {
                        audioData[(j * mDeviceChannelCount) + i] = mBuffer[j*2 + i];
                    } else if (mChannelCount ==1 ) {
                        audioData[(j * mDeviceChannelCount) + i] = mBuffer[j];
                    }
                }
            }
        } else {
            int input_frames = numFrames * mDeviceSampleRate / mSampleRate;
            std::vector<float> input;
            input.resize(input_frames * mChannelCount);
            auto read_frames = drmp3_read_pcm_frames_f32(&mMp3, input_frames, input.data());
            if (read_frames <= 0) {
                LOGV("Decode failed");
                return;
            }

            auto output = upsample_pcm(input, mSampleRate, mDeviceSampleRate);
            LOGD("MP3 input_frames %d output_frames %zu", input_frames, output.size());
            if ((int)output.size() < (int)numFrames * mChannelCount) {
                output.resize(numFrames * mChannelCount);
            }
            // Resample
            for (int j = 0; j < (int)numFrames; ++j) {
                for (int i = 0; i < mDeviceChannelCount; ++i) {
                    if (mChannelCount == 2) {
                        audioData[(j * mDeviceChannelCount) + i] = output[j*2 + i];
                    } else if (mChannelCount ==1 ) {
                        audioData[(j * mDeviceChannelCount) + i] = output[j];
                    }
                }
            }
        }
    }

    Mp3SoundGenerator(int32_t sampleRate, int32_t channelCount)
        : TappableAudioSource(sampleRate, channelCount) { }

    ~Mp3SoundGenerator() { }

    void setSampleRate(int32_t rate) {
        LOGD("MP3 setSampleRate %d", rate);
        mDeviceSampleRate = rate;
    }

  private:
    drmp3                    mMp3;
    uint64_t                 mTotalFrames;
    std::unique_ptr<float[]> mBuffer = std::make_unique<float[]>(kSharedBufferSize);
    std::mutex               mMutex;

    int32_t mDeviceSampleRate = 48000;
    int32_t mDeviceChannelCount = 2;
};
