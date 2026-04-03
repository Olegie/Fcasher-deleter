#ifndef FCASHER_CORE_CLEANUP_ENGINE_H
#define FCASHER_CORE_CLEANUP_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#include "core/file_record.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fc_cleanup_options {
    int dry_run;
    int verbose;
    const char** protected_roots;
    size_t protected_root_count;
} fc_cleanup_options;

typedef struct fc_cleanup_entry {
    char path[FC_MAX_PATH_UTF8];
    char message[FC_MAX_REASON_LEN];
    fc_category category;
    uint64_t size_bytes;
    int deleted;
    int simulated;
} fc_cleanup_entry;

typedef struct fc_cleanup_result {
    fc_cleanup_entry* items;
    size_t item_count;
    size_t item_capacity;
    size_t planned_count;
    size_t deleted_count;
    size_t skipped_count;
    uint64_t candidate_bytes;
    uint64_t freed_bytes;
} fc_cleanup_result;

void fc_cleanup_result_init(fc_cleanup_result* result);
void fc_cleanup_result_free(fc_cleanup_result* result);
int fc_cleanup_execute(const fc_file_record* records,
                       size_t record_count,
                       const fc_cleanup_options* options,
                       fc_cleanup_result* result);

#ifdef __cplusplus
}
#endif

#endif
