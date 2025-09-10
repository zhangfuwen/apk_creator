#include <dlfcn.h>
#include "ndk_utils/log.h"
using TestFunc = void (*)(void);
TestFunc rust_test_func;

void link_rust() {
    // Step 1: Open the shared library
    void* handle = dlopen("librust_lib.so", RTLD_LAZY);
    if (!handle) {
        LOGI("Error loading library: %s", dlerror());
        return;
    }

    // Step 2: Clear any existing error
    dlerror();

    // Step 3: Load the function symbol
    rust_test_func = (TestFunc)dlsym(handle, "rust_opencv_test");

    // Check for errors
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        LOGI("Error loading symbol 'rust_opencv_test': %s", dlsym_error);
        dlclose(handle);
        return;
    }

    // Step 4: Call the function
    rust_test_func();

    // Step 5: Close the library
    dlclose(handle);

    return;
}
