/*
 * octanex_ffi.cpp - extern "C" bridge layer for OctaneX.
 *
 * This file provides the C interface that Python FFI (via ctypes)
 * and other callers can use directly. It wraps the C++ module
 * and provides a stable ABI.
 */

#include <stdlib.h>
#include <string.h>

#include "../octanex_module_api/octanemoduleapi.h"
#include "octanex.h"
#include "octanex_fallback.h"
#include "octanex_module.cpp"

/* Module info bridge */
extern "C" {

OctaneModuleInfo getModuleInfo(void) {
    OctaneModuleInfo info = {0};
    info.name = getModuleName();
    info.display_name = getModuleName();
    info.module_id = "octanex";
    info.type = 0;
    info.start = modStart;
    info.stop = modStop;
    info.execute = modExecute;
    return info;
}

} /* extern "C" */
