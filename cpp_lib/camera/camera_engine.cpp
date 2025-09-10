/**
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** Description
 *   Demonstrate NDK Camera interface added to android-24
 */

#include "camera_engine.h"
#include "ndk_utils/log.h"

#include <cstdio>

/**
 * constructor and destructor for main application class
 * @param app native_app_glue environment
 * @return none
 */
CameraEngine::CameraEngine(android_app* app)
    : app_(app)
    , cameraGranted_(false)
    , rotation_(0)
    , cameraReady_(false)
    , camera_(nullptr)
    , yuvReader_(nullptr)
    , jpgReader_(nullptr) {
    memset(&savedNativeWinRes_, 0, sizeof(savedNativeWinRes_));
}

CameraEngine::~CameraEngine() {
    cameraReady_ = false;
    DeleteCamera();
}

struct android_app* CameraEngine::AndroidApp(void) const {
    return app_;
}

/**
 * Create a camera object for onboard BACK_FACING camera
 */
void CameraEngine::CreateCamera(void) {
    // Camera needed to be requested at the run-time from Java SDK
    // if Not granted, do nothing.
    if (!cameraGranted_ || !app_->window) {
        LOGW("Camera Sample requires Full Camera access");
        return;
    }

    int32_t displayRotation = GetDisplayRotation();
    rotation_ = displayRotation;

    camera_ = new NDKCamera();
    ASSERT(camera_, "Failed to Create CameraObject");

    int32_t facing = 0, angle = 0, imageRotation = 0;
    if (camera_->GetSensorOrientation(&facing, &angle)) {
        if (facing == ACAMERA_LENS_FACING_FRONT) {
            imageRotation = (angle + rotation_) % 360;
            imageRotation = (360 - imageRotation) % 360;
        } else {
            imageRotation = (angle - rotation_ + 360) % 360;
        }
    }
    LOGI("Phone Rotation: %d, Present Rotation Angle: %d", rotation_, imageRotation);
    ImageFormat view{0, 0, 0}, capture{0, 0, 0};
    camera_->MatchCaptureSizeRequest(app_->window, &view, &capture);

    ASSERT(view.width && view.height, "Could not find supportable resolution");

    // Request the necessary nativeWindow to OS
    bool portraitNativeWindow = (savedNativeWinRes_.width < savedNativeWinRes_.height);
    ANativeWindow_setBuffersGeometry(app_->window, portraitNativeWindow ? view.height : view.width,
                                     portraitNativeWindow ? view.width : view.height, WINDOW_FORMAT_RGBA_8888);

    yuvReader_ = new ImageReader(&view, AIMAGE_FORMAT_YUV_420_888);
    yuvReader_->SetPresentRotation(imageRotation);
    jpgReader_ = new ImageReader(&capture, AIMAGE_FORMAT_JPEG);
    jpgReader_->SetPresentRotation(imageRotation);
    jpgReader_->RegisterCallback(
            this, [](void* ctx, const char* str) -> void { reinterpret_cast<CameraEngine*>(ctx)->OnPhotoTaken(str); });

    // now we could create session
    camera_->CreateSession(yuvReader_->GetNativeWindow(), jpgReader_->GetNativeWindow(), imageRotation);
}

void CameraEngine::DeleteCamera(void) {
    cameraReady_ = false;
    if (camera_) {
        delete camera_;
        camera_ = nullptr;
    }
    if (yuvReader_) {
        delete yuvReader_;
        yuvReader_ = nullptr;
    }
    if (jpgReader_) {
        delete jpgReader_;
        jpgReader_ = nullptr;
    }
}

/**
 * Initiate a Camera Run-time usage request to Java side implementation
 *  [ The request result will be passed back in function
 *    notifyCameraPermission()]
 */
void CameraEngine::RequestCameraPermission() {
    if (!app_)
        return;

    JNIEnv*          env;
    ANativeActivity* activity = app_->activity;
    activity->vm->GetEnv((void**)&env, JNI_VERSION_1_6);

    activity->vm->AttachCurrentThread(&env, NULL);

    jobject activityObj = env->NewGlobalRef(activity->clazz);
    jclass  clz = env->GetObjectClass(activityObj);
    env->CallVoidMethod(activityObj, env->GetMethodID(clz, "RequestCamera", "()V"));
    env->DeleteGlobalRef(activityObj);

    activity->vm->DetachCurrentThread();
}

/**
 * Process to user's sensitivity and exposure value change
 * all values are represented in int64_t even exposure is just int32_t
 * @param code ACAMERA_SENSOR_EXPOSURE_TIME or ACAMERA_SENSOR_SENSITIVITY
 * @param val corresponding value from user
 */
void CameraEngine::OnCameraParameterChanged(int32_t code, int64_t val) {
    camera_->UpdateCameraRequestParameter(code, val);
}

/**
 * The main function rendering a frame. In our case, it is yuv to RGBA8888
 * converter
 */
void CameraEngine::DrawFrame(void) {
    if (!cameraReady_ || !yuvReader_)
        return;
    AImage* image = yuvReader_->GetNextImage();
    if (!image) {
        return;
    }

    ANativeWindow_acquire(app_->window);
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(app_->window, &buf, nullptr) < 0) {
        yuvReader_->DeleteImage(image);
        return;
    }

    yuvReader_->DisplayImage(&buf, image);
    ANativeWindow_unlockAndPost(app_->window);
    ANativeWindow_release(app_->window);
}

/**
 * Handle Android System APP_CMD_INIT_WINDOW message
 *   Request camera persmission from Java side
 *   Create camera object if camera has been granted
 */
void CameraEngine::OnAppInitWindow(void) {
    bool cameraPermission = ndkCheckCameraPermission();
    if (cameraPermission) {
        LOGI("CAMERA permission already granted");
    } else {
        LOGI("CAMERA permission not granted, requesting permission");
        RequestCameraPermission();
        cameraPermission = ndkCheckCameraPermission();
        if (cameraPermission) {
            LOGI("CAMERA permission granted");
        } else {
            LOGI("CAMERA permission not granted");
        }
    }

    cameraGranted_ = cameraPermission;
    if (!cameraGranted_) {
        LOGI("Camera permission not granted, exit");
        return;
    }

    rotation_ = GetDisplayRotation();
    LOGI("Rotation: %d", rotation_);

    CreateCamera();
    ASSERT(camera_, "CameraCreation Failed");
    LOGI("Camera created");

    //  EnableUI();

    // NativeActivity end is ready to display, start pulling images
    cameraReady_ = true;
    // LOGI("Start preview");
    // camera_->StartPreview(true);
    camera_->TakePhoto();
    LOGV("exit");
}

/**
 * Handle APP_CMD_TEMR_WINDOW
 */
void CameraEngine::OnAppTermWindow(void) {
    cameraReady_ = false;
    DeleteCamera();
}

/**
 * Handle APP_CMD_CONFIG_CHANGED
 */
void CameraEngine::OnAppConfigChange(void) {
    int newRotation = GetDisplayRotation();

    if (newRotation != rotation_) {
        OnAppTermWindow();

        rotation_ = newRotation;
        OnAppInitWindow();
    }
}

/**
 * Retrieve saved native window width.
 * @return width of native window
 */
int32_t CameraEngine::GetSavedNativeWinWidth(void) {
    return savedNativeWinRes_.width;
}

/**
 * Retrieve saved native window height.
 * @return height of native window
 */
int32_t CameraEngine::GetSavedNativeWinHeight(void) {
    return savedNativeWinRes_.height;
}

/**
 * Retrieve saved native window format
 * @return format of native window
 */
int32_t CameraEngine::GetSavedNativeWinFormat(void) {
    return savedNativeWinRes_.format;
}

/**
 * Save original NativeWindow Resolution
 * @param w width of native window in pixel
 * @param h height of native window in pixel
 * @param format
 */
void CameraEngine::SaveNativeWinRes(int32_t w, int32_t h, int32_t format) {
    savedNativeWinRes_.width = w;
    savedNativeWinRes_.height = h;
    savedNativeWinRes_.format = format;
}

int CameraEngine::GetDisplayRotation() {
    return 0;
}

void CameraEngine::OnPhotoTaken(const char* fileName) {
    LOGI("photo taken %s", fileName);
}

#include <android/permission_manager.h>

bool ndkCheckCameraPermission() {
    int32_t  outResult;
    uint32_t pid = 0;
    uint32_t uid = 0;
    auto     code = APermissionManager_checkPermission("android.permission.CAMERA", pid, uid, &outResult);
    if (code = PERMISSION_MANAGER_STATUS_OK) {
        LOGI("APermissionManager_checkPermission error: %d", code);
        return false;
    }
    if (outResult == PERMISSION_MANAGER_PERMISSION_GRANTED) {
        LOGI("CAMERA permission ok");
        return true;
    }
    LOGI("CAMERA permission not granted");
    return false;
}
