cmake_minimum_required(VERSION 3.14)
project(octanex C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directory layout:
#   octanex-module/
#   ├── CMakeLists.txt (this file)
#   ├── octanex_module_api/   ← vendored OTOY headers
#   ├── src/
#   │   ├── octanex_ffi.cpp   ← extern "C" bridge
#   │   ├── octanex_module.cpp
#   │   ├── octanex_commands.cpp
#   │   ├── octanex_fallback.cpp
#   │   └── octanex.h
#   └── modules/              ← output dylib here
include_directories(octanex_module_api)
include_directories(src)

# === Octane modules library ===
add_library(octanex SHARED
    src/octanex_ffi.cpp
    src/octanex_module.cpp
    src/octanex_commands.cpp
    src/octanex_fallback.cpp
    # Convenience wrappers from OTOY
    octanex_module_api/octanemoduleapi.cpp
)

# Link C++ stdlib for C++ runtime
if(APPLE)
    target_link_libraries(octanex PRIVATE c++)
endif()

# macOS: -undefined dynamic_lookup so we can call into Octane without
# linking against the full SDK at build time.
target_compile_options(octanex PRIVATE
    -fPIC
    -undefined dynamic_lookup
    -Wall
    -Wextra
)

# Set module output directory
set_target_properties(octanex PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/modules
    PREFIX ""                # Octane expects octanex.dylib, not liboctanex.dylib
    SUFFIX ".dylib"
    INSTALL_RPATH "@loader_path"
)

# === Optional: test target ===
if(EXISTS ${CMAKE_SOURCE_DIR}/test/test_module_loading.cpp)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    add_executable(test_module_loading
        test/test_module_loading.cpp
    )
    target_link_libraries(test_module_loading PRIVATE octanex)
endif()

# === Install target (copies dylib into Octane's modules/ dir) ===
install(TARGETS octanex DESTINATION modules)
