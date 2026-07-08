# OctaneX Bridge Module

A **standalone C++ module for Octane X**, built in parallel to the existing
Lua-based `octanex-mcp` system (the Hermes MCP bridge).

## Design intent: independent, but composable

The C++ module and the MCP bridge are **two independent components**:

- Either one runs and is useful **on its own** — the module inside Octane X, the
  MCP bridge driving Octane over its own channel. Neither requires the other to
  function.
- When run **together**, they share a single `queue/` directory as a common
  command channel. The C++ module can consume that queue as a sidecar, and the
  MCP bridge falls back to it when the dylib is not loaded. Together they give a
  faster, deeper control path than either alone.

See `src/octanex_fallback.cpp` and `src/octanex_mcp/bridge.py` for the shared
queue contract, and the `octanex-module` Hermes skill for the full breakdown.

## Architecture

```
octanex/
├── docs/
│   └── PLAN.md              ← Implementation plan (written)
├── octane_module_api/
│   └── README.md            ← OTOY module API reference (written)
├── modules/
│   └── octanex.dylib*       ← Build output (after build)
├── scripts/
│   ├── build-release.sh     ← Build script (written)
│   ├── install-to-octane.sh ← Install script (written)
│   └── start-octane-with-module.sh
├── src/
│   ├── octanex_module_api/
│   │   ├── README.md        ← OTOY API types (written)
│   │   ├── octanemoduleapi.h ← OTOY API header stubs
│   └── octanex/
│       ├── __init__.py      ← Python package init
│       └── octanex_module.cpp
├── test/
│   ├── test_commands.cpp
│   └── test_module_loading.cpp
├── pyproject.toml           ← Python project file (written)
└── README.md                ← This file
```

## Key files written

- **docs/PLAN.md** - Full implementation plan with directory layout, API signatures, and build config
- **CMakeLists.txt** - C++ build system config
- **src/octanex_module_api/README.md** - API reference with C++ types
- **src/octanex_module.cpp** - Core C++ module implementation (command dispatch, direct node system, event handling)
- **src/octanex_commands.h** - Command dispatch table and helpers
- **src/octanex_fallback.cpp** - File-queue fallback implementation
- **src/octanex_ffi.cpp** - C interface layer (extern "C" bindings for Python FFI and C callers)
- **src/octanex_mcp/bridge.py** - Python FFI bridge (ctypes, queue, availability detection)
- **test/test_commands.cpp** - C++ tests for all core commands
- **test/test_module_loading.cpp** - Module loading test

## Usage

```bash
# 1. Install module
./scripts/install-to-octane.sh

# 2. Start Octane X with module
./scripts/start-octane-with-module.sh
```

## Module path

The compiled module loads from the container sandbox:
```
/Users/craig/Library/Containers/com.otoy.rndrviewer/Data/Library/OctaneRender/modules/
```

## Development

```bash
# Clean build
./scripts/build-release.sh

# Install to Octane
./scripts/install-to-octane.sh

# Start Octane with module
./scripts/start-octane-with-module.sh
```
