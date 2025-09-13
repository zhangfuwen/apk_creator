#define NCNN_VULKAN 1
#include <android/asset_manager.h>
#include <android/input.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <android/native_window.h>
#include <cstddef>
#include <unistd.h>
#include <stdlib.h>  // for srand and rand
#include <stdint.h>

#include <jni.h>
#include <android/native_activity.h>
#include <android/permission_manager.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>

#include <dlfcn.h>
#include "ndk_utils/log.h"

#include "app_engine.hpp"
#include "java_helpers/java_helper.hpp"
#include "rust_helper.hpp"


struct android_app* gapp = nullptr;

void HandleKey(int keycode, int bDown) {
    LOGV("Key: code %d (down:%d)\n", keycode, bDown);
}

void HandleButton(int x, int y, int button, int bDown) {
    LOGV("Button(x:%d, y:%d, button:%d, bDown:%d)\n", x, y, button, bDown);
}

void HandleMotion(int x, int y, int mask) {
    LOGV("Motion: %d,%d (%d)\n", x, y, mask);
}

int32_t HandleInput(struct android_app* app, AInputEvent* event) {
    LOGV("handle_input\n");

#ifdef ANDROID
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        [[maybe_unused]] static uint64_t downmask;

        int action = AMotionEvent_getAction(event);
        int whichsource = action >> 8;
        action &= AMOTION_EVENT_ACTION_MASK;
        size_t pointerCount = AMotionEvent_getPointerCount(event);

        for (size_t i = 0; i < pointerCount; ++i) {
            int x = AMotionEvent_getX(event, i);
            int y = AMotionEvent_getY(event, i);
            int index = AMotionEvent_getPointerId(event, i);

            if (action == AMOTION_EVENT_ACTION_POINTER_DOWN || action == AMOTION_EVENT_ACTION_DOWN) {
                int id = index;
                if (action == AMOTION_EVENT_ACTION_POINTER_DOWN && id != whichsource)
                    continue;
                HandleButton(x, y, id, 1);
                downmask |= 1 << id;
                ANativeActivity_showSoftInput(app->activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
            } else if (action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_UP
                       || action == AMOTION_EVENT_ACTION_CANCEL) {
                int id = index;
                if (action == AMOTION_EVENT_ACTION_POINTER_UP && id != whichsource)
                    continue;
                HandleButton(x, y, id, 0);
                downmask &= ~(1 << id);
            } else if (action == AMOTION_EVENT_ACTION_MOVE) {
                HandleMotion(x, y, index);
            }
        }
        return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        [[maybe_unused]] int code = AKeyEvent_getKeyCode(event);
        
#    ifdef ANDROID_USE_SCANCODES
        HandleKey(code, AKeyEvent_getAction(event));
#    else
        // int unicode = AndroidGetUnicodeChar( code, AMotionEvent_getMetaState( event ) );
        // if( unicode )
        // 	HandleKey( unicode, AKeyEvent_getAction(event) );
        // else
        // {
        // 	HandleKey( code, !AKeyEvent_getAction(event) );
        // 	return (code == 4)?1:0; //don't override functionality.
        // }
#    endif

        return 1;
    }
#endif
    return 0;
}

// Main entry point
void android_main(struct android_app* app) {
    AppEngine* appEngine = new AppEngine(app);

    app->onAppCmd = AppEngine::handleCmd;
    app->onInputEvent = HandleInput;


    androidMakeFullscreen(app);
    link_rust();


    [[maybe_unused]] uint64_t update_time = 0;
    // Main loop
    while (true) {
        appEngine->processEvents();
        appEngine->processFrame();
        usleep(16000);  // ~60 FPS, Throttle loop (optional)
    }
}
