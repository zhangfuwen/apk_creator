#pragma once
#include <vector>
#include <oboe/AudioStreamCallback.h>
#include "ndk_utils/log.h"

#include "IRestartable.h"
/**
 * This is a callback object which will be called when a stream error occurs.
 *
 * It is constructed using an `IRestartable` which allows it to automatically restart the parent
 * object if the stream is disconnected (for example, when headphones are attached).
 *
 * @param IRestartable - the object which should be restarted when the stream is disconnected
 */
class DefaultErrorCallback : public oboe::AudioStreamErrorCallback {
public:

    DefaultErrorCallback(IRestartable &parent): mParent(parent) {}
    virtual ~DefaultErrorCallback() = default;

    virtual void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override {
        (void)oboeStream;
        // Restart the stream if the error is a disconnect, otherwise do nothing and log the error
        // reason.
        if (error == oboe::Result::ErrorDisconnected) {
            LOGI("Restarting AudioStream");
            mParent.restart();
        }
//        LOGE("Error was %s", oboe::convertToText(error));
    }

private:
    IRestartable &mParent;

};

