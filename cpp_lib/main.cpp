#include <android/input.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <android/native_window.h>
#include <unistd.h>
#include <stdlib.h>  // for srand and rand
#include <stdint.h>

#include <jni.h>
#include <android/native_activity.h>

#include <android/permission_manager.h>


#define LOG_TAG   "NativeOpenGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOG_LINE() __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s:%d", __FILE__, __LINE__)

struct Engine {
    android_app*   app;
    ANativeWindow* window;
    EGLDisplay     display;
    EGLSurface     surface;
    EGLContext     context;
    bool           initialized;
};
struct android_app* gapp = nullptr;

struct Color {
    float r, g, b, a;
};

volatile Color g_color = {.r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};

void change_color() {
    g_color.r = (float)rand() / (float)RAND_MAX;
    g_color.g = (float)rand() / (float)RAND_MAX;
    g_color.b = (float)rand() / (float)RAND_MAX;
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
    eglChooseConfig(engine->display, attribs, &config, 1, &numConfigs);

    // Create EGL context
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
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
    if (!engine->initialized)
        return;

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

void HandleKey(int keycode, int bDown) {
    LOGI("Key: code %d (down:%d)\n", keycode, bDown);
    //	if( keycode == 4 ) { AndroidSendToBack( 1 ); }
}

void HandleButton(int x, int y, int button, int bDown) {
    LOGI("Button(x:%d, y:%d, button:%d, bDown:%d)\n", x, y, button, bDown);
}

void HandleMotion(int x, int y, int mask) {
    LOGI("Motion: %d,%d (%d)\n", x, y, mask);
}

int32_t handle_input(struct android_app* app, AInputEvent* event) {
    LOGI("handle_input\n");
#ifdef ANDROID
    //Potentially do other things here.

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        static uint64_t downmask;

        int action = AMotionEvent_getAction(event);
        int whichsource = action >> 8;
        action &= AMOTION_EVENT_ACTION_MASK;
        size_t pointerCount = AMotionEvent_getPointerCount(event);

        for (size_t i = 0; i < pointerCount; ++i) {
            int x, y, index;
            x = AMotionEvent_getX(event, i);
            y = AMotionEvent_getY(event, i);
            index = AMotionEvent_getPointerId(event, i);

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
        int code = AKeyEvent_getKeyCode(event);
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

//Based on: https://stackoverflow.com/questions/41820039/jstringjni-to-stdstringc-with-utf8-characters

jstring android_permission_name(JNIEnv* env, const char* perm_name) {
    // nested class permission in class android.Manifest,
    // hence android 'slash' Manifest 'dollar' permission
    //    jclass ClassManifestpermission = env->FindClass( envptr, "android/Manifest$permission");
    //    jfieldID lid_PERM = env->GetStaticFieldID( envptr, ClassManifestpermission, perm_name, "Ljava/lang/String;" );
    //    jstring ls_PERM = (jstring)(env->GetStaticObjectField( envptr, ClassManifestpermission, lid_PERM ));
    const auto ClassManifestpermission = env->FindClass("android/Manifest$permission");
    LOG_LINE();
    auto       lid_PERM = env->GetStaticFieldID(ClassManifestpermission, perm_name, "Ljava/lang/String;");
    if (lid_PERM == nullptr) {
        LOGI("permission not found: %s", perm_name);
        return nullptr;
    }
    LOG_LINE();
    auto       ls_PERM = (jstring)env->GetObjectField(ClassManifestpermission, lid_PERM);
    LOG_LINE();
    return ls_PERM;
}

void AndroidMakeFullscreen() {
    //Partially based on https://stackoverflow.com/questions/47507714/how-do-i-enable-full-screen-immersive-mode-for-a-native-activity-ndk-app
    JNIEnv*  env = 0;
    JNIEnv** envptr = &env;
    JavaVM*  jvm = gapp->activity->vm;

    jvm->AttachCurrentThread(envptr, NULL);
    env = (*envptr);

    LOG_LINE();

    //Get android.app.NativeActivity, then get getWindow method handle, returns view.Window type
    jclass    activityClass = env->FindClass("android/app/NativeActivity");
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    jobject   window = env->CallObjectMethod(gapp->activity->clazz, getWindow);
    LOG_LINE();

    //Get android.view.Window class, then get getDecorView method handle, returns view.View type
    jclass    windowClass = env->FindClass("android/view/Window");
    jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
    jobject   decorView = env->CallObjectMethod(window, getDecorView);
    LOG_LINE();

    //Get the flag values associated with systemuivisibility
    jclass    viewClass = env->FindClass("android/view/View");
    const int flagLayoutHideNavigation = env->GetStaticIntField(
            viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION", "I"));
    const int flagLayoutFullscreen = env->GetStaticIntField(
            viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN", "I"));
    const int flagLowProfile =
            env->GetStaticIntField(viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_LOW_PROFILE", "I"));
    const int flagHideNavigation =
            env->GetStaticIntField(viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I"));
    const int flagFullscreen =
            env->GetStaticIntField(viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I"));
    const int flagImmersiveSticky =
            env->GetStaticIntField(viewClass, env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I"));
    LOG_LINE();

    jmethodID setSystemUiVisibility = env->GetMethodID(viewClass, "setSystemUiVisibility", "(I)V");

    //Call the decorView.setSystemUiVisibility(FLAGS)
    env->CallVoidMethod(decorView, setSystemUiVisibility,
                        (flagLayoutHideNavigation | flagLayoutFullscreen | flagLowProfile | flagHideNavigation
                         | flagFullscreen | flagImmersiveSticky));

    LOG_LINE();
    //now set some more flags associated with layoutmanager -- note the $ in the class path
    //search for api-versions.xml
    //https://android.googlesource.com/platform/development/+/refs/tags/android-9.0.0_r48/sdk/api-versions.xml

    jclass    layoutManagerClass = env->FindClass("android/view/WindowManager$LayoutParams");
    const int flag_WinMan_Fullscreen = env->GetStaticIntField(
            layoutManagerClass, (env->GetStaticFieldID(layoutManagerClass, "FLAG_FULLSCREEN", "I")));
    const int flag_WinMan_KeepScreenOn = env->GetStaticIntField(
            layoutManagerClass, (env->GetStaticFieldID(layoutManagerClass, "FLAG_KEEP_SCREEN_ON", "I")));
    const int flag_WinMan_hw_acc = env->GetStaticIntField(
            layoutManagerClass, (env->GetStaticFieldID(layoutManagerClass, "FLAG_HARDWARE_ACCELERATED", "I")));
    //    const int flag_WinMan_flag_not_fullscreen = env->GetStaticIntField(layoutManagerClass, (env->GetStaticFieldID(layoutManagerClass, "FLAG_FORCE_NOT_FULLSCREEN", "I") ));
    //call window.addFlags(FLAGS)
    env->CallVoidMethod(window, (env->GetMethodID(windowClass, "addFlags", "(I)V")),
                        (flag_WinMan_Fullscreen | flag_WinMan_KeepScreenOn | flag_WinMan_hw_acc));

    jvm->DetachCurrentThread();
    LOG_LINE();
}

/**
 * \brief Tests whether a permission is granted.
 * \param[in] app a pointer to the android app.
 * \param[in] perm_name the name of the permission, e.g.,
 *   "READ_EXTERNAL_STORAGE", "WRITE_EXTERNAL_STORAGE".
 * \retval true if the permission is granted.
 * \retval false otherwise.
 * \note Requires Android API level 23 (Marshmallow, May 2015)
 */
int AndroidHasPermissions(const char* perm_name) {
    struct android_app* app = gapp;
    // const JNIInvokeInterface ** jniiptr = app->activity->vm;
    auto* jvm = app->activity->vm;
    // const struct JNIInvokeInterface * jnii = *jniiptr;

    // if( android_sdk_version < 23 )
    // {
    // 	printf( "Android SDK version %d does not support AndroidHasPermissions\n", android_sdk_version );
    // 	return 1;
    // }

    JNIEnv*  env = 0;
    JNIEnv** envptr = &env;
    jvm->AttachCurrentThread(envptr, NULL);
    env = (*envptr);

    LOGI("AttachCurrentThread success");

    int     result = 0;
    jstring ls_PERM = android_permission_name(env, perm_name);
    if (ls_PERM == nullptr) {
        LOGI("permission not found: %s", perm_name);
        return 0;
    }
    LOG_LINE();

    LOGI("permission name got.");

    jint PERMISSION_GRANTED = (-1);

    {
        jclass   ClassPackageManager = env->FindClass("android/content/pm/PackageManager");
        jfieldID lid_PERMISSION_GRANTED = env->GetStaticFieldID(ClassPackageManager, "PERMISSION_GRANTED", "I");
        PERMISSION_GRANTED = env->GetStaticIntField(ClassPackageManager, lid_PERMISSION_GRANTED);
    }
    {
        jobject   activity = app->activity->clazz;
        jclass    ClassContext = env->FindClass("android/content/Context");
        jmethodID MethodcheckSelfPermission =
                env->GetMethodID(ClassContext, "checkSelfPermission", "(Ljava/lang/String;)I");
        jint int_result = env->CallIntMethod(activity, MethodcheckSelfPermission, ls_PERM);
        result = (int_result == PERMISSION_GRANTED);
    }

    jvm->DetachCurrentThread();

    return result;
}

/**
 * \brief Query file permissions.
 * \details This opens the system dialog that lets the user
 *  grant (or deny) the permission.
 * \param[in] app a pointer to the android app.
 * \note Requires Android API level 23 (Marshmallow, May 2015)
 */
void AndroidRequestAppPermissions(const char* perm) {
    // if( android_sdk_version < 23 )
    // {
    // 	printf( "Android SDK version %d does not support AndroidRequestAppPermissions\n",android_sdk_version );
    // 	return;
    // }

    struct android_app* app = gapp;
    JNIEnv*             env = 0;
    JNIEnv**            envptr = &env;
    JavaVM*       jvm = app->activity->vm;
    jvm->AttachCurrentThread(envptr, NULL);
    jobject activity = app->activity->clazz;

    jobjectArray perm_array = env->NewObjectArray(1, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    env->SetObjectArrayElement(perm_array, 0, android_permission_name(env, perm));
    jclass ClassActivity = env->FindClass("android/app/Activity");

    jmethodID MethodrequestPermissions =
            env->GetMethodID(ClassActivity, "requestPermissions", "([Ljava/lang/String;I)V");

    // Last arg (0) is just for the callback (that I do not use)
    env->CallVoidMethod(activity, MethodrequestPermissions, perm_array, 0);
    jvm->DetachCurrentThread();
}

/* Example:
	int hasperm = android_has_permission( "RECORD_AUDIO" );
	if( !hasperm )
	{
		android_request_app_permissions( "RECORD_AUDIO" );
	}
*/


// Main entry point
void android_main(struct android_app* state) {
    Engine engine = {};
    engine.app = state;
    engine.app->userData = &engine;
    engine.app->onAppCmd = engine_handle_cmd;
    engine.app->onInputEvent = handle_input;
    gapp = state;

    AndroidMakeFullscreen();

    int32_t outResult;
    uint32_t pid = 0;
    uint32_t uid = 0;
    APermissionManager_checkPermission("CAMERA", pid, uid, &outResult);

    LOGI("Native OpenGL app started");

    // {
    //     int perm_read_exteranl_storage = AndroidHasPermissions("android.permission.READ_EXTERNAL_STORAGE");
    //     int perm_write_exteranl_storage = AndroidHasPermissions("android.permission.WRITE_EXTERNAL_STORAGE");
    //     LOGI("perm_read_exteranl_storage: %d", perm_read_exteranl_storage);
    //     LOGI("perm_write_exteranl_storage: %d", perm_write_exteranl_storage);
    // }

    {
        int perm_read_exteranl_storage = AndroidHasPermissions("READ_EXTERNAL_STORAGE");
        int perm_write_exteranl_storage = AndroidHasPermissions("WRITE_EXTERNAL_STORAGE");
        int perm_camera = AndroidHasPermissions("CAMERA");
        LOGI("perm_read_exteranl_storage: %d", perm_read_exteranl_storage);
        LOGI("perm_write_exteranl_storage: %d", perm_write_exteranl_storage);
        LOGI("perm_camera: %d", perm_camera);
        if (!perm_camera) {
            AndroidRequestAppPermissions("CAMERA");
            int perm_camera = AndroidHasPermissions("CAMERA");
            LOGI("perm_camera: %d", perm_camera);
        }
    }

    int                         events;
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
        usleep(16000);  // ~60 FPS
    }
}
