/*
 * octanex_fallback.h - C-compatible fallback queue implementation.
 *
 * Provides file-queue-based implementations of OctaneX operations
 * as a complement to the direct C++ API path.
 */

#ifndef OCTANEX_FALLBACK_H
#define OCTANEX_FALLBACK_H

#ifdef __cplusplus
#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstdlib>

namespace OctaneX {
namespace Fallback {
    std::string file_queue_execute(const char* op, const char* payload);
} /* namespace Fallback */
} /* namespace OctaneX */
#else /* !__cplusplus */
int  octane_queue_push(const void* entry);
int  octane_queue_pop(void* entry);
int  octane_queue_clear(void);
int  octane_queue_empty(void);
#endif /* __cplusplus */

#endif /* OCTANEX_FALLBACK_H */
