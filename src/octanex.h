/*
 * octanex.h - Unified header for the Octanex C++ module.
 *
 * Public interface for the OctaneX module.
 *
 * DESIGN INTENT (parallel component):
 *   This module is a standalone Octane X native module, built in parallel to the
 *   Lua-based octanex-mcp (Hermes MCP) bridge. The two components are fully
 *   independent:
 *     - the module runs inside Octane X on its own, and the MCP bridge drives
 *       Octane X over its own channel; neither depends on the other.
 *     - when run together they converge on a shared `queue/` directory as a
 *       common command channel (see octanex_fallback.cpp), so the module can act
 *       as a sidecar for the bridge and the bridge can fall back to the queue
 *       when the dylib is not loaded.
 *   Together they expose a deeper/faster control path than either alone.
 */

#ifndef OCTANEX_H
#define OCTANEX_H

/* Include the main API definitions */
#include "../octanex_module_api/octanemoduleapi.h"

/* Result codes */
#define OCTANEX_SUCCESS        0
#define OCTANEX_ERROR         -1
#define OCTANEX_ERROR_NOT_FOUND -2

/* C++ bindings */
#ifdef __cplusplus

namespace OctaneX {
void g_version_init();
} /* namespace OctaneX */

#endif /* __cplusplus */

#endif /* OCTANEX_H */
