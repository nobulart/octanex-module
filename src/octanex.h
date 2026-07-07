/*
 * octanex.h — Unified header for the Octanex C++ module.
 *
 * Provides the public interface for the OctaneX module. This header is
 * compatible with both the legacy build and the new C++ module.
 */

#ifndef OCTANEX_H
#define OCTANEX_H

#include <stddef.h>

/*
 * Common return codes (used by octanex_module.cpp)
 */
#define OCTANEX_SUCCESS            0
#define OCTANEX_ERROR             -1
#define OCTANEX_ERROR_NOT_FOUND   -2
#define OCTANEX_ERROR_INVALID_ARG -3

#ifdef __cplusplus

/* C++ additions */
#include <string>
#include <vector>

typedef std::string string_t;
typedef std::vector<string_t> string_list_t;

extern "C" {

/* C-compatible types */
typedef struct {
    float x, y, z;
} vec3;

typedef struct {
    float x, y, z, w;
} vec4;

} /* extern "C" */

/* OTOY module registration */
typedef struct {
    const char* name;
    const char* display_name;
    const char* module_id;
    const char* type;
} OtoyModuleInfo;

#endif /* __cplusplus */

#endif /* OCTANEX_H */
