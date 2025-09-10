#define DR_MP3_IMPLEMENTATION
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
#include <memory>

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

#include "ndk_utils/log.h"
#include "ndk_utils/util.h"


#include "audio/OboeEngine.h"
#include "camera/CameraController.hpp"
#include "camera/camera_engine.h"

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
                    appEngine->initDisplay();
                    appEngine->playMp3();
                    appEngine->m_oboeEngine.start();
                    // if (!appEngine->m_camCtrl.openAndCapture("0")) {
                    //     LOGE("Failed to open camera");
                    // }
                    appEngine->m_camEngine = new CameraEngine(appEngine->m_app);
                    appEngine->m_camEngine->SaveNativeWinRes(ANativeWindow_getWidth(app->window),
                                                             ANativeWindow_getHeight(app->window),
                                                             ANativeWindow_getFormat(app->window));
                    appEngine->m_camEngine->OnAppInitWindow();
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
        //update();
        //draw();
        return true;
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
        m_initialized = true;
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
            //eye_renderer->resize(800, 600);
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
            playMp3();
            m_lastAnimationTime = now;
            if (rand() % 2 == 0) {
                m_eyeRenderer->playBlink(1.0f);
            } else {
                m_eyeRenderer->playSleepy(3.0f);
            }
            m_eyeRenderer->playMouth();
        }
        m_eyeRenderer->update((float)delta_time_second);
        m_2dScene->update();
    }

    // Render loop: clear screen to blue
    void draw() {
        if (!m_initialized)
            return;

        //glClearColor(g_color.r, g_color.g, g_color.b, g_color.a);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //        scene->draw();
        m_eyeRenderer->render();
        eglSwapBuffers(m_display, m_surface);
    }

    void playMp3() {
        if (std::get<0>(m_mp3Data) == nullptr) {
            m_mp3Data = readAsset("test.mp3");
            if (std::get<0>(m_mp3Data) == nullptr) {
                LOGE("Failed to read asset: %s", "test.mp3");
                return;
            }
        }
        m_oboeEngine.playMp3((uint8_t*)std::get<0>(m_mp3Data)->data(), std::get<1>(m_mp3Data));
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
        LOGI("buf %p, size %d", buf->data(), size);
        auto readCount = AAsset_read(asset, buf->data(), size);
        LOGI("readCount %d", readCount);
        if (readCount != size) {
            LOGE("Failed to read asset: %s", filepath.c_str());
            return {nullptr, 0};
        }
        AAsset_close(asset);
        return {buf, size};
    }

  private:
    android_app* m_app;
    bool         m_initialized;

    ANativeWindow* m_window;
    EGLDisplay     m_display;
    EGLSurface     m_surface;
    EGLContext     m_context;

    OboeEngine       m_oboeEngine;
    CameraController m_camCtrl;
    CameraEngine*    m_camEngine;

    renderer_2d::Scene* m_2dScene;
    EyeRenderer*        m_eyeRenderer;

    double m_lastUpdateTime = 0.0f;
    double m_lastAnimationTime = 0.0f;

    std::tuple<std::shared_ptr<std::vector<uint8_t>>, size_t> m_mp3Data = {nullptr, 0};
};
