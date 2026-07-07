/*
 * octanex_fallback.h — C-compatible fallback queue implementation.
 *
 * Provides file-queue-based implementations of OctaneX operations
 * as a complement to the direct C++ API path.
 */

#ifndef OCTANEX_FALLBACK_H
#define OCTANEX_FALLBACK_H

#include <stddef.h>

/* ---- C-compatible type aliases ---- */
#ifdef __cplusplus
typedef const char* (*string_fn_t)(const char*);
typedef int (*int_fn_t)(const char*, const char*);
typedef struct OctaneXModule OctaneXModule;
#else
typedef const char* string_fn_t;
typedef int int_fn_t;
struct OctaneXModule {
    int (*handle)(OctaneXModule* self, const char* op, const char* payload);
};
#endif

#ifdef __cplusplus
/* C++ functions */
namespace OctaneX {
namespace Fallback {

int write_queue_command(const char* op, const char* payload, const char* path);
int read_queue_command(char* op, char* payload, size_t op_max_len, size_t payload_max_len, const char* path);
std::string file_queue_execute(const char* op, const char* payload);
std::string get_next_obj_name();
std::string get_json_value(const char* json, const char* key);

typedef std::string (*fn_write_queue)(const char*, const char*, const char*);
typedef std::string (*fn_read_queue)(const char*, const char*);
typedef std::string (*fn_get_next)(void);

} /* namespace Fallback */
} /* namespace OctaneX */
#else
/* C functions */
int octanex_write_queue_command(const char* op, const char* payload, const char* path);
int octanex_read_queue_command(char* op, char* payload, size_t op_max_len, size_t payload_max_len, const char* path);
const char* octanex_file_queue_execute(const char* op, const char* payload);
const char* octanex_get_next_obj_name(void);
const char* octanex_get_json_value(const char* json, const char* key);

typedef const char* (*cfn_write_queue)(const char*, const char*, const char*);
typedef const char* (*cfn_read_queue)(const char*, const char*);
typedef const char* (*cfn_get_next)(void);
#endif

/* ---- C entry points (always available) ---- */
#ifdef __cplusplus
extern "C" {
#endif

int octanex_fallback_write_command(const char* op, const char* payload, const char* path);
int octanex_fallback_read_command(char* op, char* payload, size_t op_max_len, size_t payload_max_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCTANEX_FALLBACK_H */
