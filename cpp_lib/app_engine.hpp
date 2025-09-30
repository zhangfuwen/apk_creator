#include <unordered_map>
#define DR_MP3_IMPLEMENTATION 1
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
#include <tuple>
#include <set>
#include <memory>
#include <future>

#include <jni.h>
#include <android/native_activity.h>
#include <android/permission_manager.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>

#include <dlfcn.h>
#include "renderer/eye_renderer.h"
#include "renderer/gl.h"
#include "renderer/texture_renderer.hpp"

#include "ndk_utils/log.h"
#include "ndk_utils/util.h"
#include "ndk_utils/data_types.h"

#include "audio/OboeEngine.h"
#include "camera/CameraController.hpp"
#include "camera/camera_engine.h"

#include "ndk_utils/util.h"
PRINT_MACRO(NCNN_VULKAN);
#include "ai/yolov8.h"
PRINT_MACRO(NCNN_VULKAN);

#include "ncnn/gpu.h"

#include "input_handler.h"


using Mp3Data = std::tuple<std::shared_ptr<std::vector<uint8_t>>, size_t>;

bool pointInRect(float x, float y, float x1, float y1, float w, float h) {
    return x > x1 && x < x1 + w && y > y1 && y < y1+h;
}

struct AppEngine {
    AppEngine(android_app* app) : m_app(app) { app->userData = this; }

    ~AppEngine() { delete m_2dScene; }

    // Handle commands from main thread
    static void handleCmd(android_app* app, int32_t cmd) {
        LOGI("entry, app: %p, window: %p, cmd: %d", app, app->window, cmd);
        auto appEngine = (AppEngine*)app->userData;
        auto cam = appEngine->m_camEngine;

        switch (cmd) {
            case APP_CMD_SAVE_STATE:
                LOGI("APP_CMD_SAVE_STATE");
                break;

            case APP_CMD_INIT_WINDOW:
                {
                    LOGI("APP_CMD_INIT_WINDOW");
                    if (app->window == nullptr) {
                        LOGE("Failed to open window");
                        break;
                    }
                    appEngine->m_window = app->window;
#if (!RENDER_CAM_TO_WINDOW)
                    appEngine->initDisplay();
#endif  // RENDER_CAM_TO_WINDOW

                    appEngine->playMp3();

                    //                    appEngine->m_yoloInit = std::async(std::launch::async, [appEngine]() {
                    appEngine->initYolo();
                    //                    });
                    AConfiguration* config = AConfiguration_new();
                    AConfiguration_fromAssetManager(config, app->activity->assetManager);
                    appEngine->m_screenWidth = AConfiguration_getScreenWidthDp(config);
                    appEngine->m_screenHeight = AConfiguration_getScreenHeightDp(config);
                    appEngine->m_oboeEngine.start();
                    // if (!appEngine->m_camCtrl.openAndCapture("0")) {
                    //     LOGE("Failed to open camera");
                    // }
                    appEngine->m_camEngine = new CameraEngine(appEngine->m_app);
                    appEngine->m_camEngine->SaveNativeWinRes(ANativeWindow_getWidth(app->window),
                                                             ANativeWindow_getHeight(app->window),
                                                             ANativeWindow_getFormat(app->window));
                    appEngine->m_camEngine->OnAppInitWindow();
                    appEngine->m_rgba.bits = nullptr;
                    appEngine->m_initialized = true;

                    // auto yolov8 = new YOLOv8_det_coco();
                    // auto assetMgr = app->activity->assetManager;
                    // yolov8->load(assetMgr, "yolov8n.param", "yolov8n.bin", true);
                    // yolov8->set_det_target_size(320);
                    // yolov8->detect(const cv::Mat &rgb, std::vector<Object> &objects);
                    // appEngine->m_yolov8 = yolov8;
                }
                break;
            case APP_CMD_START:
                LOGI("APP_CMD_START");
                break;
            case APP_CMD_RESUME:
                LOGI("APP_CMD_RESUME");
                break;

            case APP_CMD_TERM_WINDOW:
                LOGI("APP_CMD_TERM_WINDOW");
                appEngine->terminateDisplay();
                appEngine->m_oboeEngine.stop();
                appEngine->m_window = nullptr;
                appEngine->m_camEngine->OnAppTermWindow();
                ANativeWindow_setBuffersGeometry(app->window, cam->GetSavedNativeWinWidth(),
                                                 cam->GetSavedNativeWinHeight(), cam->GetSavedNativeWinFormat());
                break;
            case APP_CMD_CONFIG_CHANGED:
                LOGI("APP_CMD_CONFIG_CHANGED");
                cam->OnAppConfigChange();
                break;

            case APP_CMD_DESTROY:
                LOGI("APP_CMD_DESTROY");
                app->userData = nullptr;
                break;
            default:
                LOGI("unknown cmd: %d", cmd);
                break;
        }
        LOGI("exit");
    }

    void processEvents() {
        int                         events;
        struct android_poll_source* source;
        while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) {
                source->process(m_app, source);
            }

            if (m_app->destroyRequested != 0) {
                terminateDisplay();
                return;
            }
        }
    }

    bool processFrame() {
        if (!m_initialized) {
            return false;
        }
        update();
        draw();
        return true;
    }

    void handleKey(int keycode, int bDown) {
        LOGV("Key: code %d (down:%d)\n", keycode, bDown);
        for (auto handler : m_inputHandlers) {
            handler->handleKey(keycode, bDown);
        }
    }

    void handleButton(int x, int y, int button, int bDown) {
        LOGD("Button(x:%d, y:%d, button:%d, bDown:%d)\n", x, y, button, bDown);
        x -= m_screenWidth / 2;
        y -= m_screenHeight / 2;
        float x_norm = (float)x / m_screenWidth;
        float y_norm = (float)y / m_screenHeight;
        LOGV("Button(x:%f, y:%f, button:%d, bDown:%d)\n", x_norm, y_norm, button, bDown);
        if (pointInRect(x_norm, y_norm, -1, -1, 1,1)) { playMp3("robot_boot.mp3"); }
        if (pointInRect(x_norm, y_norm, 0, -1, 1,1)) { playMp3("robot_random_code.mp3"); }
        if (pointInRect(x_norm, y_norm, -1, 0, 1,1)) { playMp3("robot_thankyou.mp3"); }
        if (pointInRect(x_norm, y_norm, 0, 0, 1,1)) { playMp3("test.mp3"); }
        for (auto handler : m_inputHandlers) {
            handler->handleButton(x_norm, y_norm, button, bDown);
        }
    }

    void handleMotion(int x, int y, int mask) {
        LOGD("Motion: %d,%d (%d)\n", x, y, mask);
        x -= m_screenWidth / 2;
        y -= m_screenHeight / 2;
        float x_norm = (float)x / m_screenWidth;
        float y_norm = (float)y / m_screenHeight;

        for (auto handler : m_inputHandlers) {
            handler->handleMotion(x_norm, y_norm, mask);
        }
    }

  private:
    bool initDisplay() {
        // Setup EGL
        m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(m_display, 0, 0);

        // Choose config (RGB 565, depth 16)
        const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                                  EGL_OPENGL_ES2_BIT,
                                  EGL_SURFACE_TYPE,
                                  EGL_WINDOW_BIT,
                                  EGL_BLUE_SIZE,
                                  8,
                                  EGL_GREEN_SIZE,
                                  8,
                                  EGL_RED_SIZE,
                                  8,
                                  EGL_DEPTH_SIZE,
                                  16,
                                  EGL_NONE};

        EGLint    numConfigs;
        EGLConfig config;
        eglChooseConfig(m_display, attribs, &config, 1, &numConfigs);

        // Create EGL context
        const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        m_context = eglCreateContext(m_display, config, nullptr, context_attribs);

        // Create surface
        m_surface = eglCreateWindowSurface(m_display, config, m_window, nullptr);

        // Make current
        if (eglMakeCurrent(m_display, m_surface, m_surface, m_context) == EGL_FALSE) {
            LOGI("Failed to eglMakeCurrent");
            return false;
        }

        LOGI("OpenGL ES initialized");
        compileProgromOnce();
        return true;
    }

    // Terminate EGL
    void terminateDisplay() {
        if (m_display != EGL_NO_DISPLAY) {
            eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (m_context != EGL_NO_CONTEXT) {
                eglDestroyContext(m_display, m_context);
            }
            if (m_surface != EGL_NO_SURFACE) {
                eglDestroySurface(m_display, m_surface);
            }
            eglTerminate(m_display);
            m_display = EGL_NO_DISPLAY;
            m_context = EGL_NO_CONTEXT;
            m_surface = EGL_NO_SURFACE;
        }
    }

    void compileProgromOnce() {
        if (m_2dScene == nullptr) {
            m_2dScene = new renderer_2d::Scene();
            m_2dScene->addRectangle(std::make_shared<renderer_2d::Rectangle>(0, 0, 0.2, 0.2));
            m_2dScene->addRectangle(std::make_shared<renderer_2d::Rectangle>(0.5, 0.5, 0.2, 0.2));
            m_2dScene->addRectangle(std::make_shared<renderer_2d::Rectangle>(0.8, 0.8, 0.2, 0.2));
            m_eyeRenderer = new EyeRenderer();
            m_inputHandlers.push_back(m_eyeRenderer);
            //eye_renderer->resize(800, 600);
            m_textureRenderer = new TextureRenderer();
            if (!m_textureRenderer->init()) {
                LOGE("init texture renderer failed");
                delete m_textureRenderer;
                m_textureRenderer = nullptr;
            }
        }
    }

    void update() {
        auto now = get_time_second();

        double delta_time_second = now - m_lastUpdateTime;
        m_lastUpdateTime = now;
        // if (delta_time_second > 0.1f) {
        //     LOGE("delta_time_second: %f", delta_time_second);
        // }
        if (now - m_lastAnimationTime > 5.0f) {
            //           oboeEngine.tap(true);
            m_lastAnimationTime = now;
            // if (rand() % 2 == 0) {
            //     m_eyeRenderer->playBlink(0.5f);
            // } else {
            //     m_eyeRenderer->playSleepy(1.0f);
            // }
            // m_eyeRenderer->playMouth();
        }
        m_eyeRenderer->update((float)delta_time_second);
        //m_2dScene->update();
    }

    // Render loop: clear screen to blue
    void draw() {
#if RENDER_CAM_TO_WINDOW
        /*
        m_camEngine->DrawFrame([this](uint8_t* data, int32_t width, int32_t height, int32_t stride) {
            YoloProcesser(data, width, height, stride);
        });
        */
        m_camEngine->DrawFrame();

#endif
        if (!m_initialized)
            return;


#if (!RENDER_CAM_TO_WINDOW)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // if (m_camEngine == nullptr) {
        //     LOGE("m_camEngine == nullptr");
        //     return;
        // }
        // m_rgba.format = WINDOW_FORMAT_RGBA_8888;
        // if(m_rgba.bits == nullptr) {
        //     m_rgba.width = m_rgba.stride = 640;
        //     m_rgba.height = 480;
        //     m_rgba.bits = new uint8_t[m_rgba.width * m_rgba.height * 4];
        // }
        // auto ret = m_camEngine->GetImageData(m_rgba);
        // if (ret == 0) {
        //     YoloProcesser((uint8_t*)m_rgba.bits, m_rgba.width, m_rgba.height, m_rgba.stride*4);
        //     m_textureRenderer->loadTexture(m_rgba.bits, m_rgba.width, m_rgba.height);
        // }

        //m_2dScene->draw();

        m_eyeRenderer->render();
        //m_textureRenderer->render();
        eglSwapBuffers(m_display, m_surface);
#endif
    }

    void playMp3(std::string name = "test.mp3") {
        Mp3Data mp3Data = {nullptr, 0};
        if (!m_audioData.contains(name)) {
            if (!m_audioNames.contains(name)) {
                LOGE("unknown audio name");
                return;
            }
            mp3Data = readAsset(name);
            if (std::get<0>(mp3Data) == nullptr) {
                LOGE("Failed to read asset: %s", name.c_str());
                return;
            }
            m_audioData[name] = mp3Data;
        }
        m_oboeEngine.playMp3((uint8_t*)std::get<0>(mp3Data)->data(), std::get<1>(mp3Data));
    }


  private:
    void assetTest() {
        auto [text, text_size] = readAsset("test.txt");
        LOGI("asset text(size %ld): %s", text_size, (char*)text->data());
    }

    std::tuple<std::shared_ptr<std::vector<uint8_t>>, size_t> readAsset(std::string filepath) {
        AAsset* asset = AAssetManager_open(m_app->activity->assetManager, filepath.c_str(), AASSET_MODE_BUFFER);
        if (asset == nullptr) {
            LOGE("Failed to open asset: %s", filepath.c_str());
            return {nullptr, 0};
        }
        size_t size = AAsset_getLength(asset);
        size_t size2 = AAsset_getRemainingLength(asset);
        LOGI("size %lu, size2 %lu", size, size2);
        auto buf = std::make_shared<std::vector<uint8_t>>(size);
        LOGI("buf %p, size %zu", buf->data(), size);
        auto readCount = AAsset_read(asset, buf->data(), size);
        LOGI("readCount %d", readCount);
        if (readCount != (int)size) {
            LOGE("Failed to read asset: %s", filepath.c_str());
            return {nullptr, 0};
        }
        AAsset_close(asset);
        return {buf, size};
    }

    void initYolo() {
        if (m_yolov8 == nullptr) {
            LOGI("YoloProcesser m_yolov8 == nullptr, create m_yolov8");
            auto yolo = new YOLOv8_det_oiv7;
            //yolo = new YOLOv8_det_coco;
            // if (taskid == 1) m_yolov8 = new YOLOv8_det_oiv7;
            // if (taskid == 2) m_yolov8 = new YOLOv8_seg;
            // if (taskid == 3) m_yolov8 = new YOLOv8_pose;
            // if (taskid == 4) m_yolov8 = new YOLOv8_cls;
            // if (taskid == 5) m_yolov8 = new YOLOv8_obb;
            //std::string parampath = "yolov8n_pnnx.py.ncnn.param";
            //std::string modelpath = "yolov8n_pnnx.py.ncnn.bin";
            std::string parampath = "yolov8n_oiv7.ncnn.param";
            std::string modelpath = "yolov8n_oiv7.ncnn.bin";

            bool use_gpu = true;
            bool use_turnip = false;
            if (use_turnip) {
                ncnn::create_gpu_instance("libvulkan_freedreno.so");
            } else if (use_gpu) {
                ncnn::create_gpu_instance();
            }

            LOGI("load yolo: %s, %s", parampath.c_str(), modelpath.c_str());
            auto ret = yolo->load(m_app->activity->assetManager, parampath.c_str(), modelpath.c_str(),
                                  use_gpu || use_turnip);
            if (ret != 0) {
                LOGE("Failed to load yolo: %d", ret);
                return;
            }
            yolo->set_det_target_size(320);
            LOGI("load yolo ok");
            m_yolov8 = yolo;
        }
    }

    /**
     * YoloProcesser
     * @param rgba: rgba data, width * height * 4
     * @param width: image width
     * @param height: image height
     * @param stride: image stride, in bytes
     */
    void YoloProcesser(uint8_t* rgba, int32_t width, int32_t height, int32_t stride) {
        LOGV("entry, data:%p, %d, %d, %d", rgba, width, height, stride);
        if (m_yolov8 == nullptr) {
            LOGE("YoloProcesser m_yolov8 == nullptr");
            return;
        }

        cv::Mat rgb(height, width, CV_8UC3);

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                rgb.at<cv::Vec3b>(i, j)[0] = rgba[i * stride + j * 4 + 0];
                rgb.at<cv::Vec3b>(i, j)[1] = rgba[i * stride + j * 4 + 1];
                rgb.at<cv::Vec3b>(i, j)[2] = rgba[i * stride + j * 4 + 2];
            }
        }
        std::vector<Object> objects;
        m_yolov8->detect(rgb, objects);
        if (!objects.empty()) {
            LOGI("detected %zu objects", objects.size());
            LOGI("first: %d, %f, %f, %f, %f", objects[0].label, objects[0].rect.x, objects[0].rect.y,
                 objects[0].rect.width, objects[0].rect.height);
        }

        m_yolov8->draw(rgb, objects);

        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                rgba[row * stride + col * 4 + 0] = rgb.at<cv::Vec3b>(row, col)[0];
                rgba[row * stride + col * 4 + 1] = rgb.at<cv::Vec3b>(row, col)[1];
                rgba[row * stride + col * 4 + 2] = rgb.at<cv::Vec3b>(row, col)[2];
            }
        }
    }

  private:
    android_app* m_app = nullptr;
    bool         m_initialized = false;

    ANativeWindow* m_window = nullptr;
    EGLDisplay     m_display;
    EGLSurface     m_surface;
    EGLContext     m_context;

    int32_t m_screenWidth = 0;
    int32_t m_screenHeight = 0;

    std::set<std::string> m_audioNames = {
        "test.mp3",
        "robot_boot.mp3",
        "robot_thankyou.mp3",
        "robot_random_code.mp3"
    };
    std::unordered_map<std::string, Mp3Data> m_audioData;
    OboeEngine       m_oboeEngine;
    CameraController m_camCtrl;
    CameraEngine*    m_camEngine = nullptr;

    std::vector<InputHandler*> m_inputHandlers;
    renderer_2d::Scene*        m_2dScene = nullptr;
    EyeRenderer*               m_eyeRenderer = nullptr;

    double m_lastUpdateTime = 0.0f;
    double m_lastAnimationTime = 0.0f;


    YOLOv8*           m_yolov8 = nullptr;
    std::future<void> m_yoloInit;

    TextureRenderer*     m_textureRenderer = nullptr;
    std::vector<uint8_t> mTextureData;  // Keep alive
#if (!RENDER_CAM_TO_WINDOW)
    ANativeWindow_Buffer m_rgba;
#endif
};
