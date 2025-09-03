#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <android/native_window.h>
#include <unistd.h>
#include <stdlib.h> // for srand and rand
#include <stdint.h>

#define LOG_TAG "NativeOpenGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct Engine {
    android_app* app;
    ANativeWindow* window;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    bool initialized;
};

struct Color {
    float r,g,b,a;
};

volatile Color g_color = {
    .r = 1.0f,
    .g = 0.0f,
    .b = 0.0f,
    .a = 1.0f
};

void change_color() {
    g_color.r = (float) rand() / (float) RAND_MAX;
    g_color.g = (float) rand() / (float) RAND_MAX;
    g_color.b = (float) rand() / (float) RAND_MAX;
}

uint64_t get_time_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Initialize EGL
bool engine_init_display(Engine* engine) {
    // Setup EGL
    engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(engine->display, 0, 0);

    // Choose config (RGB 565, depth 16)
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    EGLint numConfigs;
    EGLConfig config;
    eglChooseConfig(engine->display, attribs, &config, 1, &numConfigs);

    // Create EGL context
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    engine->context = eglCreateContext(engine->display, config, nullptr, context_attribs);

    // Create surface
    engine->surface = eglCreateWindowSurface(engine->display, config, engine->window, nullptr);

    // Make current
    if (eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context) == EGL_FALSE) {
        LOGI("Failed to eglMakeCurrent");
        return false;
    }

    LOGI("OpenGL ES initialized");
    engine->initialized = true;
    return true;
}

// Render loop: clear screen to blue
void engine_draw(Engine* engine) {
    if (!engine->initialized) return;

    glClearColor(g_color.r, g_color.g, g_color.b, g_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);
}

// Terminate EGL
void engine_term_display(Engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
        engine->display = EGL_NO_DISPLAY;
        engine->context = EGL_NO_CONTEXT;
        engine->surface = EGL_NO_SURFACE;
    }
}

// Handle commands from main thread
void engine_handle_cmd(android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            break;

        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != nullptr) {
                engine->window = engine->app->window;
                engine_init_display(engine);
            }
            break;

        case APP_CMD_TERM_WINDOW:
            engine_term_display(engine);
            engine->window = nullptr;
            break;

        case APP_CMD_DESTROY:
            engine->app->userData = nullptr;
            break;
    }
}

// Main entry point
void android_main(struct android_app* state) {
    Engine engine = {};
    engine.app = state;
    engine.app->userData = &engine;
    engine.app->onAppCmd = engine_handle_cmd;

    LOGI("Native OpenGL app started");

    int events;
    struct android_poll_source* source;

    uint64_t update_time = 0;
    // Main loop
    while (true) {
        while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) {
                source->process(state, source);
            }

            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        // Only draw if initialized
        if (engine.initialized) {
            uint64_t now = get_time_millis();
            if (now - update_time > 1000) {
                change_color();
                update_time = now;
            }
            engine_draw(&engine);
        }

        // Throttle loop (optional)
        usleep(16000); // ~60 FPS
    }
}
