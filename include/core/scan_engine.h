#ifndef FCASHER_CORE_SCAN_ENGINE_H
#define FCASHER_CORE_SCAN_ENGINE_H

#include <stddef.h>

#include "core/path_filters.h"
#include "core/scan_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fc_scan_options {
    const char** protected_roots;
    size_t protected_root_count;
    int verbose;
} fc_scan_options;

int fc_scan_targets(const fc_scan_target* targets,
                    size_t target_count,
                    const fc_scan_options* options,
                    fc_scan_result* result);

#ifdef __cplusplus
}
#endif

#endif
