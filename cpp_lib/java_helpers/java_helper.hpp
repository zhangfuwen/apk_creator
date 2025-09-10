#include <jni.h>
#include <android_native_app_glue.h>
#include "ndk_utils/log.h"

/* Example:
	int hasperm = android_has_permission( "RECORD_AUDIO" );
	if( !hasperm )
	{
		android_request_app_permissions( "RECORD_AUDIO" );
	}
*/

inline jstring androidPermissionName(JNIEnv* env, const char* perm_name) {
    // nested class permission in class android.Manifest,
    // hence android 'slash' Manifest 'dollar' permission
    //    jclass ClassManifestpermission = env->FindClass( envptr, "android/Manifest$permission");
    //    jfieldID lid_PERM = env->GetStaticFieldID( envptr, ClassManifestpermission, perm_name, "Ljava/lang/String;" );
    //    jstring ls_PERM = (jstring)(env->GetStaticObjectField( envptr, ClassManifestpermission, lid_PERM ));
    const auto ClassManifestpermission = env->FindClass("android/Manifest$permission");
    LOG_LINE();
    auto lid_PERM = env->GetStaticFieldID(ClassManifestpermission, perm_name, "Ljava/lang/String;");
    if (lid_PERM == nullptr) {
        LOGI("permission not found: %s", perm_name);
        return nullptr;
    }
    LOG_LINE();
    auto ls_PERM = (jstring)env->GetStaticObjectField(ClassManifestpermission, lid_PERM);
    LOG_LINE();
    return ls_PERM;
}

inline void androidMakeFullscreen(android_app* app) {
    // Partially based on 
    // https://stackoverflow.com/questions/47507714/how-do-i-enable-full-screen-immersive-mode-for-a-native-activity-ndk-app
    JNIEnv*  env = 0;
    JNIEnv** envptr = &env;
    JavaVM*  jvm = app->activity->vm;

    jvm->AttachCurrentThread(envptr, NULL);
    env = (*envptr);

    LOG_LINE();

    //Get android.app.NativeActivity, then get getWindow method handle, returns view.Window type
    jclass    activityClass = env->FindClass("android/app/NativeActivity");
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    jobject   window = env->CallObjectMethod(app->activity->clazz, getWindow);
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
inline int androidHasPermissions(android_app * app, const char* perm_name) {
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
    jstring ls_PERM = androidPermissionName(env, perm_name);
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
inline void androidRequestAppPermissions(android_app* app, const char* perm) {
    // if( android_sdk_version < 23 )
    // {
    // 	printf( "Android SDK version %d does not support AndroidRequestAppPermissions\n",android_sdk_version );
    // 	return;
    // }

    JNIEnv*             env = 0;
    JNIEnv**            envptr = &env;
    JavaVM*             jvm = app->activity->vm;
    jvm->AttachCurrentThread(envptr, NULL);
    jobject activity = app->activity->clazz;

    jobjectArray perm_array = env->NewObjectArray(1, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    env->SetObjectArrayElement(perm_array, 0, androidPermissionName(env, perm));
    jclass ClassActivity = env->FindClass("android/app/Activity");

    jmethodID MethodrequestPermissions =
            env->GetMethodID(ClassActivity, "requestPermissions", "([Ljava/lang/String;I)V");

    // Last arg (0) is just for the callback (that I do not use)
    env->CallVoidMethod(activity, MethodrequestPermissions, perm_array, 0);
    jvm->DetachCurrentThread();
}
//Based on: https://stackoverflow.com/questions/41820039/jstringjni-to-stdstringc-with-utf8-characters

