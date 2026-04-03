#ifndef FCASHER_CORE_PATH_FILTERS_H
#define FCASHER_CORE_PATH_FILTERS_H

#include <stddef.h>
#include <stdint.h>

#include "core/file_record.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fc_scan_target {
    const char* root_path;
    fc_category category;
    const char* removable_reason;
    const char** suffixes;
    size_t suffix_count;
    const char** name_contains;
    size_t name_contains_count;
    uint32_t modified_within_days;
    int recursive;
    int include_all_files;
} fc_scan_target;

int fc_path_normalize_copy(const char* input, char* output, size_t output_size);
int fc_path_is_under_root(const char* path, const char* root);
int fc_path_is_protected(const char* path);
int fc_path_matches_suffix(const char* path, const char* suffix);
int fc_path_matches_any_suffix(const char* path, const char** suffixes, size_t suffix_count);
int fc_path_matches_any_token(const char* value, const char** tokens, size_t token_count);
int fc_record_matches_target(const char* path, const fc_scan_target* target, uint64_t last_write_unix);

#ifdef __cplusplus
}
#endif

#endif
