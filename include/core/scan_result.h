#ifndef FCASHER_CORE_SCAN_RESULT_H
#define FCASHER_CORE_SCAN_RESULT_H

#include <stddef.h>
#include <stdint.h>

#include "core/file_record.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fc_skip_record {
    char path[FC_MAX_PATH_UTF8];
    char reason[FC_MAX_REASON_LEN];
} fc_skip_record;

typedef struct fc_scan_result {
    fc_file_record* files;
    size_t file_count;
    size_t file_capacity;
    fc_skip_record* skipped;
    size_t skipped_count;
    size_t skipped_capacity;
    char** scanned_directories;
    size_t scanned_directory_count;
    size_t scanned_directory_capacity;
    uint64_t total_size_bytes;
} fc_scan_result;

void fc_scan_result_init(fc_scan_result* result);
void fc_scan_result_free(fc_scan_result* result);
int fc_scan_result_add_file(fc_scan_result* result, const fc_file_record* record);
int fc_scan_result_add_skipped(fc_scan_result* result, const char* path, const char* reason);
int fc_scan_result_add_scanned_directory(fc_scan_result* result, const char* path);
size_t fc_scan_result_count_for_category(const fc_scan_result* result, fc_category category);
uint64_t fc_scan_result_size_for_category(const fc_scan_result* result, fc_category category);

#ifdef __cplusplus
}
#endif

#endif
