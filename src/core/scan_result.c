#include "core/scan_result.h"

#include <stdlib.h>
#include <string.h>

static int fc_ascii_case_equal(const char* left, const char* right) {
    size_t index = 0;
    if (left == NULL || right == NULL) {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        char lc = left[index];
        char rc = right[index];
        if (lc >= 'A' && lc <= 'Z') {
            lc = (char) (lc - 'A' + 'a');
        }
        if (rc >= 'A' && rc <= 'Z') {
            rc = (char) (rc - 'A' + 'a');
        }
        if (lc != rc) {
            return 0;
        }
        ++index;
    }

    return left[index] == '\0' && right[index] == '\0';
}

static int fc_reserve(void** buffer, size_t element_size, size_t current_capacity, size_t required_capacity) {
    void* new_buffer;
    size_t new_capacity = current_capacity == 0U ? 16U : current_capacity;

    while (new_capacity < required_capacity) {
        new_capacity *= 2U;
    }

    new_buffer = realloc(*buffer, element_size * new_capacity);
    if (new_buffer == NULL) {
        return 0;
    }

    *buffer = new_buffer;
    return (int) new_capacity;
}

static int fc_copy_string(char** destination, const char* value) {
    size_t length;
    char* new_value;

    if (destination == NULL || value == NULL) {
        return 0;
    }

    length = strlen(value);
    new_value = (char*) malloc(length + 1U);
    if (new_value == NULL) {
        return 0;
    }

    memcpy(new_value, value, length + 1U);
    *destination = new_value;
    return 1;
}

void fc_scan_result_init(fc_scan_result* result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
}

void fc_scan_result_free(fc_scan_result* result) {
    size_t index;

    if (result == NULL) {
        return;
    }

    free(result->files);
    free(result->skipped);

    if (result->scanned_directories != NULL) {
        for (index = 0; index < result->scanned_directory_count; ++index) {
            free(result->scanned_directories[index]);
        }
    }

    free(result->scanned_directories);
    memset(result, 0, sizeof(*result));
}

int fc_scan_result_add_file(fc_scan_result* result, const fc_file_record* record) {
    size_t index;
    int new_capacity;

    if (result == NULL || record == NULL) {
        return 0;
    }

    for (index = 0; index < result->file_count; ++index) {
        if (fc_ascii_case_equal(result->files[index].path, record->path)) {
            return 1;
        }
    }

    if (result->file_count == result->file_capacity) {
        new_capacity = fc_reserve((void**) &result->files,
                                  sizeof(fc_file_record),
                                  result->file_capacity,
                                  result->file_count + 1U);
        if (new_capacity == 0) {
            return 0;
        }
        result->file_capacity = (size_t) new_capacity;
    }

    result->files[result->file_count++] = *record;
    result->total_size_bytes += record->size_bytes;
    return 1;
}

int fc_scan_result_add_skipped(fc_scan_result* result, const char* path, const char* reason) {
    int new_capacity;
    fc_skip_record* record;

    if (result == NULL || path == NULL || reason == NULL) {
        return 0;
    }

    if (result->skipped_count == result->skipped_capacity) {
        new_capacity = fc_reserve((void**) &result->skipped,
                                  sizeof(fc_skip_record),
                                  result->skipped_capacity,
                                  result->skipped_count + 1U);
        if (new_capacity == 0) {
            return 0;
        }
        result->skipped_capacity = (size_t) new_capacity;
    }

    record = &result->skipped[result->skipped_count++];
    memset(record, 0, sizeof(*record));
    strncpy(record->path, path, FC_MAX_PATH_UTF8 - 1);
    strncpy(record->reason, reason, FC_MAX_REASON_LEN - 1);
    return 1;
}

int fc_scan_result_add_scanned_directory(fc_scan_result* result, const char* path) {
    size_t index;
    int new_capacity;

    if (result == NULL || path == NULL) {
        return 0;
    }

    for (index = 0; index < result->scanned_directory_count; ++index) {
        if (fc_ascii_case_equal(result->scanned_directories[index], path)) {
            return 1;
        }
    }

    if (result->scanned_directory_count == result->scanned_directory_capacity) {
        new_capacity = fc_reserve((void**) &result->scanned_directories,
                                  sizeof(char*),
                                  result->scanned_directory_capacity,
                                  result->scanned_directory_count + 1U);
        if (new_capacity == 0) {
            return 0;
        }
        result->scanned_directory_capacity = (size_t) new_capacity;
    }

    if (!fc_copy_string(&result->scanned_directories[result->scanned_directory_count], path)) {
        return 0;
    }

    ++result->scanned_directory_count;
    return 1;
}

size_t fc_scan_result_count_for_category(const fc_scan_result* result, fc_category category) {
    size_t count = 0U;
    size_t index;

    if (result == NULL) {
        return 0U;
    }

    for (index = 0; index < result->file_count; ++index) {
        if (result->files[index].category == category) {
            ++count;
        }
    }

    return count;
}

uint64_t fc_scan_result_size_for_category(const fc_scan_result* result, fc_category category) {
    uint64_t total = 0ULL;
    size_t index;

    if (result == NULL) {
        return 0ULL;
    }

    for (index = 0; index < result->file_count; ++index) {
        if (result->files[index].category == category) {
            total += result->files[index].size_bytes;
        }
    }

    return total;
}
