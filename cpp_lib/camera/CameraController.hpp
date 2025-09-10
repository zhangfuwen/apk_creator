// CameraController.h
#pragma once

#include <android/log.h>
#include <android/native_activity.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>
#include <media/NdkMediaError.h>
#include <sys/stat.h>
#include <fstream>
#include <thread>

#include "ndk_utils/log.h"

/*
 * Camera ---> ACaptureSession (ACaptureSessionOutput) ---> ACaptureRequest(ACameraOutputTarget) -> ImageReader
 *                                  |                    |                                   |
 *                                 |                     |                                   |
 *                                 ---------------|      |      |----------------------------
 *                                                  ANativeWindow
 *
 * | Aspect           | ACaptureSessionOutput    | ACameraOutputTarget          |
 * | ---------------- | ------------------------ | ---------------------------- |
 * | **Scope**        | Session-level            | Request-level                |
 * | **Purpose**      | Define available outputs | Specify request destinations |
 * | **When Created** | During session setup     | During request creation      |
 * | **Lifetime**     | Tied to session duration | Tied to request duration     |
 * | **Relationship** | "What outputs exist"     | "Where to send this capture" | 
 */


// Structure to hold camera information
struct CameraInfo {
    std::string      id;
    ACameraDevice*   device = nullptr;
    ACameraMetadata* metadata = nullptr;
    bool             isBackFacing = false;
    int32_t          sensorOrientation = 0;
};

class CameraController {
  public:
    CameraController() = default;

    ~CameraController() { close(); }

    // Get list of all camera IDs
    std::vector<std::string> getCameraIds() {
        std::vector<std::string> cameraIds;

        if (!mCameraManager) {
            LOGE("Camera manager not initialized");
            return cameraIds;
        }

        ACameraIdList*  cameraIdList = nullptr;
        camera_status_t status = ACameraManager_getCameraIdList(mCameraManager, &cameraIdList);

        if (status != ACAMERA_OK || !cameraIdList) {
            LOGE("Failed to get camera list: %d", status);
            return cameraIds;
        }

        for (int i = 0; i < cameraIdList->numCameras; ++i) {
            cameraIds.push_back(cameraIdList->cameraIds[i]);
        }

        ACameraManager_deleteCameraIdList(cameraIdList);
        return cameraIds;
    }

    // Get camera metadata
    ACameraMetadata* getCameraMetadata(const std::string& cameraId) {
        if (!mCameraManager) {
            return nullptr;
        }

        ACameraMetadata* metadata = nullptr;
        camera_status_t  status = ACameraManager_getCameraCharacteristics(mCameraManager, cameraId.c_str(), &metadata);

        if (status != ACAMERA_OK) {
            LOGE("Failed to get metadata for camera %s: %d", cameraId.c_str(), status);
            return nullptr;
        }

        return metadata;
    }

    // Print camera metadata information
    void printCameraMetadata(const std::string& cameraId, ACameraMetadata* metadata) {
        if (!metadata) {
            LOGE("No metadata for camera %s", cameraId.c_str());
            return;
        }

        LOGI("=== Camera ID: %s ===", cameraId.c_str());

        // Get lens facing direction
        acamera_metadata_enum_android_lens_facing_t lensFacing = ACAMERA_LENS_FACING_FRONT;
        ACameraMetadata_const_entry                 entry = {};
        if (ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry) == ACAMERA_OK) {
            lensFacing = static_cast<acamera_metadata_enum_android_lens_facing_t>(entry.data.u8[0]);
            const char* facingStr = "UNKNOWN";
            switch (lensFacing) {
                case ACAMERA_LENS_FACING_FRONT:
                    facingStr = "FRONT";
                    break;
                case ACAMERA_LENS_FACING_BACK:
                    facingStr = "BACK";
                    break;
                case ACAMERA_LENS_FACING_EXTERNAL:
                    facingStr = "EXTERNAL";
                    break;
            }
            LOGI("Lens facing: %s", facingStr);
        }

        // Get sensor orientation
        if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &entry) == ACAMERA_OK) {
            LOGI("Sensor orientation: %d degrees", entry.data.i32[0]);
        }

        // Get available stream configurations
        if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry)
            == ACAMERA_OK) {
            LOGI("Available stream configurations: %d entries", entry.count);
        }

        // Get available capabilities
        if (ACameraMetadata_getConstEntry(metadata, ACAMERA_REQUEST_AVAILABLE_CAPABILITIES, &entry) == ACAMERA_OK) {
            LOGI("Available capabilities: %d entries", entry.count);
            for (uint32_t i = 0; i < entry.count; i++) {
                const char* capStr = "UNKNOWN";
                switch (entry.data.u8[i]) {
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE:
                        capStr = "BACKWARD_COMPATIBLE";
                        break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR:
                        capStr = "MANUAL_SENSOR";
                        break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING:
                        capStr = "MANUAL_POST_PROCESSING";
                        break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_RAW:
                        capStr = "RAW";
                        break;
                        //                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_: capStr = "PRIVATE_REPROCESSING"; break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS:
                        capStr = "READ_SENSOR_SETTINGS";
                        break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE:
                        capStr = "BURST_CAPTURE";
                        break;
                        //                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING: capStr = "YUV_REPROCESSING"; break;
                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT:
                        capStr = "DEPTH_OUTPUT";
                        break;
                        //                    case ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO: capStr = "CONSTRAINED_HIGH_SPEED_VIDEO"; break;
                }
                LOGI("  Capability[%d]: %s", i, capStr);
            }
        }

        // Get supported hardware level
        if (ACameraMetadata_getConstEntry(metadata, ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL, &entry) == ACAMERA_OK) {
            const char* levelStr = "UNKNOWN";
            switch (entry.data.u8[0]) {
                case ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
                    levelStr = "LEGACY";
                    break;
                case ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
                    levelStr = "LIMITED";
                    break;
                case ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
                    levelStr = "FULL";
                    break;
                case ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_3:
                    levelStr = "LEVEL_3";
                    break;
                case ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_EXTERNAL:
                    levelStr = "EXTERNAL";
                    break;
            }
            LOGI("Hardware level: %s", levelStr);
        }

        LOGI("====================================\n");
    }

    bool isSurfaceValid(ANativeWindow* surface) {
        if (!surface) {
            LOGE("Surface is null");
            return false;
        }

        int32_t width = ANativeWindow_getWidth(surface);
        int32_t height = ANativeWindow_getHeight(surface);
        int32_t format = ANativeWindow_getFormat(surface);

        LOGI("Surface info: %dx%d, format: %d", width, height, format);
        return width > 0 && height > 0;
    }

    // Start camera and capture one JPEG
    bool openAndCapture(const char* cameraId = "0", const char* outputPath = "/sdcard/Download/captured.jpg") {
        mOutputPath = outputPath;

        // Create manager
        mCameraManager = ACameraManager_create();
        if (!mCameraManager) {
            LOGE("Failed to create ACameraManager");
            return false;
        }
        LOGI("Camera manager created");

        auto ids = getCameraIds();
        LOGI("Found %zu cameras", ids.size());

        for (const auto& cameraId : ids) {
            ACameraMetadata* metadata = getCameraMetadata(cameraId);
            if (metadata) {
                printCameraMetadata(cameraId, metadata);
                ACameraMetadata_free(metadata);
            }
        }

        // Open camera
        if (auto status = ACameraManager_openCamera(mCameraManager, cameraId, &mDeviceCallbacks, &(mCameraDevice));
            status != ACAMERA_OK) {
            LOGE("Failed to open camera: %s", cameraId);
            return false;
        }
        LOGI("Camera device opened");

        // Create output container
        if (auto status = ACaptureSessionOutputContainer_create(&mOutputContainer); status != ACAMERA_OK) {
            LOGE("Failed to create output container");
            return false;
        }
        LOGI("Output container created");

        // Create ImageReader (JPEG, 640x480)
        if (auto status = AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG, 2, &mImageReader); status != AMEDIA_OK) {
            LOGE("Failed to create image reader");
            return false;
        }
        if (auto status = AImageReader_setImageListener(mImageReader, &(mImageListener)); status != AMEDIA_OK) {
            LOGE("Failed to set image listener");
            return false;
        }
        LOGI("Image reader created");

        if (auto status = AImageReader_getWindow(mImageReader, &mWindow); status != AMEDIA_OK) {
            LOGE("Failed to get window");
            return false;
        }
        LOGI("Window created");
        if(!isSurfaceValid(mWindow)) {
            LOGE("Invalid surface");
            return false;
        }

        // // Add preview output (required even if not used)
        // if (auto status = ACaptureSessionOutput_create(mWindow, &mPreviewOutput); status != ACAMERA_OK) {
        //     LOGE("Failed to create preview output");
        //     return false;
        // }
        // if (auto status = ACaptureSessionOutputContainer_add(mOutputContainer, mPreviewOutput); status != ACAMERA_OK) {
        //     LOGE("Failed to add preview output");
        //     return false;
        // }
        // LOGI("Preview output added");

        // Add capture output
        if (auto status = ACaptureSessionOutput_create(mWindow, &mImageOutput); status != ACAMERA_OK) {
            LOGE("Failed to create image output");
            return false;
        }
        if (auto status = ACaptureSessionOutputContainer_add(mOutputContainer, mImageOutput); status != ACAMERA_OK) {
            LOGE("Failed to add image output");
            return false;
        }
        LOGI("Image output added");

        // Create capture session
        if (auto status = ACameraDevice_createCaptureSession(mCameraDevice, mOutputContainer, &mSessionCallbacks,
                                                             &mCaptureSession);
            status != ACAMERA_OK) {
            LOGE("Failed to create capture session");
            return false;
        }
        // Cleanup (session will be managed by the system once created)
        ACaptureSessionOutput_free(mImageOutput);
        ACaptureSessionOutputContainer_free(mOutputContainer);
        LOGI("Capture session created");
        LOGI("Opening camera: %s", cameraId);
        return true;
    }

    // Graceful shutdown
    void close() {
        if (mCaptureSession) {
            ACameraCaptureSession_close(mCaptureSession);
            mCaptureSession = nullptr;
        }
        if (mCameraDevice) {
            ACameraDevice_close(mCameraDevice);
            mCameraDevice = nullptr;
        }
        if (mImageReader) {
            AImageReader_delete(mImageReader);
            mImageReader = nullptr;
        }
        if (mOutputContainer) {
            ACaptureSessionOutputContainer_free(mOutputContainer);
            mOutputContainer = nullptr;
        }
        if (mCameraManager) {
            ACameraManager_delete(mCameraManager);
            mCameraManager = nullptr;
        }
        LOGI("Camera closed");
    }

  private:
    // Called when image is available
    static void imageCallback(void* context, AImageReader* /*reader*/) {
        auto*          self = static_cast<CameraController*>(context);
        AImage*        image = nullptr;
        media_status_t status = AImageReader_acquireNextImage(self->mImageReader, &image);
        if (status != AMEDIA_OK || !image) {
            LOGE("Failed to acquire image");
            return;
        }

        uint8_t* data = nullptr;
        int      len = 0;
        AImage_getPlaneData(image, 0, &data, &len);

        if (data && len > 0) {
            self->saveImage(data, len);
        }

        AImage_delete(image);

        // Stop capture and exit
        if (self->mCaptureSession) {
            ACameraCaptureSession_stopRepeating(self->mCaptureSession);
        }

        // Optional: delay slightly before finishing
        //        std::this_thread::sleep_for(std::chrono::seconds(1));
        //       ANativeActivity_finish(self->mNativeActivity);
    }

    // Save JPEG data to file
    void saveImage(const uint8_t* data, size_t len) {
        std::ofstream file(mOutputPath, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(data), len);
            file.close();
            LOGI("Image saved to %s", mOutputPath.c_str());
        } else {
            LOGE("Failed to write image: %s", mOutputPath.c_str());
        }
    }

    // Session active → send capture request
    static void onSessionActive(void* context, ACameraCaptureSession* session) {
        LOGI("Capture session is active");
        auto* self = static_cast<CameraController*>(context);
        self->mCaptureSession = session;

        // Create capture request
        auto status = ACameraDevice_createCaptureRequest(self->mCameraDevice, TEMPLATE_ZERO_SHUTTER_LAG,
                                                         &self->mCaptureRequest);
        if (status != ACAMERA_OK) {
            LOGE("Failed to create capture request");
            return;
        }
        LOGI("Capture request created");

        // Add capture output
        ACameraOutputTarget_create(self->mWindow, &self->mCameraOutputTarget);
        ACaptureRequest_addTarget(self->mCaptureRequest, self->mCameraOutputTarget);

        // Take picture
        ACameraCaptureSession_capture(session, nullptr, 1, &self->mCaptureRequest, nullptr);
        LOGI("Capture request sent");
    }

    static void onSessionClosed(void* /*context*/, ACameraCaptureSession* session) {
        LOGI("Capture session closed");
        ACameraCaptureSession_close(session);
    }

    // Camera error
    static void onCameraError(void* context, ACameraDevice* device, int error) {
        auto* self = static_cast<CameraController*>(context);
        LOGE("Camera error: %d", error);
        if (self->mCameraDevice == device) {
            self->mCameraDevice = nullptr;
        }
        ACameraDevice_close(device);
    }

    // Device callbacks
    static ACameraDevice_stateCallbacks         mDeviceCallbacks;
    static ACameraCaptureSession_stateCallbacks mSessionCallbacks;

    // State
    ACameraManager*                 mCameraManager = nullptr;
    ACameraDevice*                  mCameraDevice = nullptr;
    ACaptureSessionOutputContainer* mOutputContainer = nullptr;
    ACaptureSessionOutput*          mPreviewOutput = nullptr;
    ACaptureSessionOutput*          mImageOutput = nullptr;
    ACameraOutputTarget*            mCameraOutputTarget = nullptr;
    ACameraCaptureSession*          mCaptureSession = nullptr;
    ACaptureRequest*                mCaptureRequest = nullptr;
    AImageReader*                   mImageReader = nullptr;
    std::string                     mOutputPath = "/sdcard/Download/captured.jpg";
    ANativeActivity*                mNativeActivity = nullptr;  // Can be set if needed for finish()
    ANativeWindow*                  mWindow = nullptr;
    AImageReader_ImageListener mImageListener = {.context = nullptr, .onImageAvailable = [](auto* ctx, auto* reader) {
                                                     auto* self = static_cast<CameraController*>(ctx);
                                                     LOGI("Image available");
                                                     self->imageCallback(ctx, reader);
                                                 }};
};

ACameraDevice_stateCallbacks CameraController::mDeviceCallbacks = {.context = nullptr,
                                                                   .onDisconnected =
                                                                           [](auto* ctx, auto* dev) {
                                                                               LOGW("Camera device disconnected");
                                                                               CameraController::onCameraError(ctx, dev,
                                                                                                               0);
                                                                           },
                                                                   .onError = CameraController::onCameraError};

ACameraCaptureSession_stateCallbacks CameraController::mSessionCallbacks = {
        .context = nullptr,
        .onClosed = CameraController::onSessionClosed,
        .onReady = [](auto...) { LOGI("Capture session is ready"); },
        .onActive = CameraController::onSessionActive,
};
