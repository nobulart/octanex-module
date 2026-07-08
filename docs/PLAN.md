---
title: OctaneX MCP Bridge Module — Parallel Fork Development Plan
target_dir: .
branch: feat/octanex-module
created: 2026-07-06
---

# OctaneX MCP Bridge Module — Parallel Fork Development Plan

## Goal

Build an Octane X C++ Module (`octanex.d.dylib`) that bridges Hermes Agent commands
directly into Octane's C++ API — in parallel with the existing Lua/file-based bridge.
Zero modification to the existing codebase. Forked as a sibling directory `octanex-module/`.

## Directory Structure

```
octanex-mcp/                           ← existing bridge (unchanged)
├── src/octanex_mcp/
├── queue/
├── processed/
├── failed/
└── ...

octanex-module/                        ← new module (forked, parallel)
├── CMakeLists.txt                     ← builds octanex.d.dylib
├── octanex_module_api/                ← vendored OTOY headers
│   ├── apimodule.h                    ← module registration macros
│   ├── api.h                          ← base API (Vec3, Vec4, etc.)
│   ├── apiGraph.h                     ← node graph
│   ├── apiNode.h                      ← OctaneNode
│   ├── apiRenderEngine.h              ← render control
│   ├── apiMaterial.h                  ← material API
│   ├── apiScene.h                     ← scene graph / geometry
│   ├── octanemoduleapi.h              ← convenience single-include
│   └── octanemoduleapi.cpp
├── src/
│   ├── octanex_ffi.cpp                ← extern "C" bridge functions (entry points)
│   ├── octanex_module.cpp             ← core module implementation
│   ├── octanex_commands.h             ← command dispatch table
│   └── octanex_fallback.cpp           ← Lua bridge fallback (file-based)
├── scripts/
│   ├── build-release.sh               ← CMake build for module
│   ├── install-to-octane.sh           ← copy .dylib to Octane X modules/
│   └── start-octane-with-module.sh    ← test launcher (with --module-log)
├── test/
│   ├── test_module_loading.cpp        ← verifies module loads in standalone
│   └── test_commands.cpp              ← tests core commands
└── docs/
    ├── API.md                         ← API surface for Python FFI
    └── DEVELOPMENT.md                 ← notes for the module branch
```

## Design Decisions

1. **No shared code** — `octanex-module/` is entirely self-contained.
   The existing `octanex-mcp/` Python bridge is untouched. The new module either:
   - Runs standalone (its own entry points), or
   - Falls back to the file-based queue for the same commands.

2. **Dual ABI** — `octanex_module.cpp` uses OTOY's `octanewrap` wrappers
   (C++ API). `octanex_ffi.cpp` exposes `extern "C"` functions for any Python FFI
   caller to use if needed later.

3. **Independent but composable** — the module and the `octanex-mcp`
   (Hermes MCP) bridge are separate components with no shared code. Each runs
   alone; when both are deployed they rendezvous on the shared `queue/`
   directory (see `octanex_fallback.cpp` / `bridge.py`). The queue is the only
   coupling, so either can be present or absent without breaking the other.

4. **Module ID** — Request `octanex` as a unique module ID from OTOY.
   Until assigned, use `0xFFFFFFFF` (auto-assigned) or a placeholder.

5. **Command dispatch** — The module receives commands via:
   - Direct API calls (for module-inside operations), or
   - The file-based queue (for external Python bridge compatibility)

6. **CMake build** — matches OTOY's `build-modules.sh` convention.

## API Surface (extern "C" — for Python FFI or other callers)

```c
// === Module Lifecycle ===
// Called by Octane at startup. Returns 0 on success.
int octanex_start(void);

// Called by Octane at shutdown.
void octanex_stop(void);

// === Command Interface (used by module menu or FFI) ===
// Execute a command by name+payload. Returns result JSON string (caller free with free()).
char* octanex_command(const char* op, const char* payload_json);

// === Core Commands (direct C++ API calls) ===
// Each returns a status code: 0 = success, >0 = error.
int octanex_cmd_import_geometry(const char* path, const char* name);
int octanex_cmd_create_material(const char* name, float r, float g, float b, float a,
                                 float roughness, float metallic);
int octanex_cmd_assign_material(const char* object_name, const char* material_name);
int octanex_cmd_set_camera(float px, float py, float pz, float tx, float ty, float tz, float fov);
int octanex_cmd_start_render(int samples, int width, int height);
int octanex_cmd_save_preview(const char* path, int samples);
int octanex_cmd_scene_summary(char* out_json, size_t max_len);

// === Scene Graph Queries (new — not in Lua bridge) ===
// Returns list of objects as JSON array (caller free).
char* octanex_list_objects(char* out_json, size_t max_len);

// Returns list of materials as JSON array (caller free).
char* octanex_list_materials(char* out_json, size_t max_len);

// Query a node's property by name (caller free).
char* octanex_get_node_property(const char* node_name, const char* property_name);

// === Fallback: use file-based JSON queue (if module is sidecar) ===
int octanex_use_file_queue(void);   // switch to file-based commands
int octanex_use_direct_api(void);   // switch to direct C++ API

// === Memory management ===
void octanex_free(void* ptr);       // OTOY's free (or stdlib free)
```

## Development Phases

### Phase 1: Core Module (2-3 weeks)

**Deliverable**: A working `octanex.d.dylib` that loads in Octane X and processes all core commands.

Steps:
1. **Set up** `octanex-module/` directory with CMakeLists.txt and vendored headers
2. **Implement** `octanex.c` with extern C bridge layer
3. **Implement** `octanex_module.cpp` with core module logic (start/stop/register)
4. **Implement** command dispatch for all 12 `ALLOWED_OPS` from `schema.py`
5. **Test** loading in Octane X standalone (verify via `--module-log`)

### Phase 2: Scene Graph Queries (1 week)

**Deliverable**: Node system access with query API.

Steps:
1. Implement `octanex_list_objects()` — walk the Octane scene graph
2. Implement `octanex_list_materials()` — enumerate materials
3. Implement `octanex_get_node_property()` — query a node's property
4. Extend `octanex_command()` to route to these new APIs

### Phase 3: Event Callbacks (1 week)

**Deliverable**: Events fire back to Python/FFI automatically.

Steps:
1. Register event callbacks in `start()` (render progress, node changes)
2. Implement `octanex_send_event(const char* event_type, const char* data_json)`
3. Add event dispatch table in Python side

### Phase 4: Work Pane Module (optional, 1-2 weeks)

**Deliverable**: Dockable canvas panel in Octane X showing live Hermes output.

Steps:
1. Implement `WorkPane` module alongside `Command` module in the same dylib
2. Add UI canvas showing preview PNG or live render
3. Register alongside the Command module

## Key API Details (from OTOY docs)

### Module Registration

```cpp
// In octanex_module.cpp
extern "C" void octanex_start() {
    // Register both Command and WorkPane modules
    APIMODULE_REGISTER_MODULE(OctaneXModule, "OctaneX Bridge", "OctanexModule", "COMMAND");
}

extern "C" void octanex_stop() {
    // Cleanup
}
```

### Command Module Entry

```cpp
class OctaneXModule : public OctaneCommandModule {
public:
    virtual void execute(void) override;
    virtual const char* getName(void) const override;
    virtual const char* getModuleName(void) const override;
    
    void handle_command(const char* op, const char* payload_json);
    
private:
    // Store references to Octane's manager objects
    ApiProjectManager* project_mgr;
    ApiRenderEngine* render_engine;
    ApiScene* scene;
};
```

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.14)
project(octanex C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OTOY headers (vendored or system)
include_directories(octanex_module_api)
link_directories(octanex_module_api)

# Module library
add_library(octanex MODULE
    src/octanex.c
    src/octanex_module.cpp
    src/octanex_commands.h
    src/octanex_fallback.cpp
    # Include OTOY's convenience wrappers
    octanex_module_api/octanemoduleapi.cpp
)

target_compile_options(octanex PRIVATE
    -fPIC
    -undefined dynamic_lookup   # Required for macOS
)

# Output to modules/ directory
set_target_properties(octanex PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/modules
    PREFIX ""                  # Octane expects .dylib not liboctanex.dylib
)

# Install target
install(TARGETS octanex DESTINATION modules)
```

### Build Script

```bash
#!/bin/bash
set -e
cd "$(dirname "$0")"/..

mkdir -p build
cd build

cmake .. \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release \
    -DOCTANE_MODULES_API_DIR="../octanex_module_api"

make -j$(nproc)

# Verify the .dylib
otool -L modules/octanex.dylib
```

## Python FFI Contract (for later integration)

When the Python bridge (existing `octanex-mcp/`) wants to delegate to the C++ module:

```python
import ctypes
import os
from pathlib import Path

def load_module():
    """Load the octanex module dylib."""
    module_path = Path("~/.Octane/modules/octanex.d.dylib").expanduser()
    if not module_path.exists():
        # Fallback: use file queue
        return None
    
    lib = ctypes.CDLL(str(module_path))
    return lib

# Set up FFI signatures
def setup_ffi(lib):
    lib.octanex_start.restype = ctypes.c_int
    lib.octanex_start.argtypes = []
    
    lib.octanex_command.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.octanex_command.restype = ctypes.c_char_p
    
    lib.octanex_free.argtypes = [ctypes.c_void_p]
    lib.octanex_free.restype = None

def execute_via_module(op: str, payload_json: str) -> str:
    """Execute a command through the C++ module."""
    lib = load_module()
    if lib is None:
        return execute_via_file_queue(op, payload_json)  # fallback
    
    result = lib.octanex_command(op.encode(), payload_json.encode())
    return ctypes.pythonapi.octanex_free(result)
```

## Installation to Octane X

```bash
# After build, install to Octane's modules directory
./scripts/install-to-octane.sh

# Check it loads
/Applications/Octane\ X.app/Contents/MacOS/Octane\ X --module-log
# Or from terminal:
/System/Applications/Octane\ X.app/Contents/MacOS/Octane\ X
```

## Verification Steps

1. **Build test**: `./scripts/build-release.sh` produces `modules/octanex.dylib`
2. **Load test**: Run `Octane X` with `--module-log`, verify "Octanex Module loaded" message
3. **Command test**: Execute `octanex_command("import_geometry", '{"path":"cube.obj"}')` — verify geometry appears
4. **Query test**: Call `octanex_list_objects()` — verify correct scene objects returned
5. **Fallback test**: Disable module, fall back to file queue — verify same results
6. **Python test**: Call `octanex_command()` via FFI from Python — verify C++ module responds

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Module loading crashes Octane | Use `OCTANE_API_ASSERT_MESSAGE_THREAD` in all API calls; separate error log file |
| C++ ABI compatibility | `extern "C"` bridge layer isolates C++ specifics |
| macOS dylib loading | `-undefined dynamic_lookup` at compile; test `otool -L` |
| Threading constraints | All API calls from message thread; module uses `pthread` for background work |
| OTOY API changes | Vendored headers version-controlled; module versioned against specific Octane release |
| File queue overlap | Module checks `queue/` for files; file-based bridge checks module is loaded before using FFI |

## Future Integration Points

1. **Python FFI** — existing `bridge.py` can detect the module and use it for hot path
2. **MCP tool integration** — `server.py` could expose `octane_module_status()` and `octane_module_execute()` tools
3. **Work Pane UI** — bridge the live canvas to show what Hermes is generating
4. **Event callbacks** — Python receives render progress, node changes, and material updates
