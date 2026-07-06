/*
 * C++ test module loader.
 *
 * Loads the OctaneX module dylib and verifies it is loaded correctly.
 * Can be run standalone to test module loading without launching Octane X.
 */

#include "../octanex_module_api/octanemoduleapi.h"
#include <iostream>
#include <dlfcn.h>
#include <cstring>

// Function pointers for the module's extern "C" functions
typedef int (*start_fn)(void);
typedef void (*stop_fn)(void);
typedef char* (*command_fn)(const char*, const char*);
typedef void (*free_fn)(void*);

// Test the module loading
int test_module_loading() {
    // Load the module dylib
    void* handle = dlopen("./modules/octanex.dylib", RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to load module: " << dlerror() << std::endl;
        return -1;
    }
    
    // Get function pointers
    start_fn start = (start_fn)dlsym(handle, "octanex_start");
    stop_fn stop = (stop_fn)dlsym(handle, "octanex_stop");
    command_fn cmd = (command_fn)dlsym(handle, "octanex_command");
    free_fn free_fn = (free_fn)dlsym(handle, "octanex_free");
    
    if (!start || !stop || !cmd || !free_fn) {
        std::cerr << "Failed to get function pointers" << std::endl;
        dlclose(handle);
        return -1;
    }
    
    // Start the module
    int result = start();
    if (result == 0) {
        std::cout << "Module loaded successfully!" << std::endl;
    } else {
        std::cerr << "Module start failed with code: " << result << std::endl;
        dlclose(handle);
        return -1;
    }
    
    // Execute a command
    const char* op = "import_geometry";
    const char* payload = "{\"path\":\"cube.obj\",\"name\":\"test\"}";
    char* result_json = cmd(op, payload);
    std::cout << "Command result: " << result_json << std::endl;
    
    // Free the result
    free_fn(result_json);
    
    // Stop the module
    stop();
    
    // Close the module
    dlclose(handle);
    
    std::cout << "Module unloaded." << std::endl;
    return 0;
}

int main() {
    int result = test_module_loading();
    return result;
}
