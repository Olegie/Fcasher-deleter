#include "core/cleanup_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "core/path_filters.h"

static int fc_cleanup_reserve(fc_cleanup_result* result, size_t required_capacity) {
    fc_cleanup_entry* new_items;
    size_t new_capacity = result->item_capacity == 0U ? 16U : result->item_capacity;

    while (new_capacity < required_capacity) {
        new_capacity *= 2U;
    }

    new_items = (fc_cleanup_entry*) realloc(result->items, new_capacity * sizeof(fc_cleanup_entry));
    if (new_items == NULL) {
        return 0;
    }

    result->items = new_items;
    result->item_capacity = new_capacity;
    return 1;
}

static int fc_cleanup_is_protected(const char* path, const fc_cleanup_options* options) {
    size_t index;

    if (fc_path_is_protected(path)) {
        return 1;
    }

    if (options == NULL) {
        return 0;
    }

    for (index = 0; index < options->protected_root_count; ++index) {
        if (options->protected_roots[index] != NULL &&
            fc_path_is_under_root(path, options->protected_roots[index])) {
            return 1;
        }
    }

    return 0;
}

static void fc_cleanup_copy(char* destination, size_t destination_size, const char* value) {
    if (destination == NULL || destination_size == 0U) {
        return;
    }

    destination[0] = '\0';
    if (value == NULL) {
        return;
    }

    strncpy(destination, value, destination_size - 1U);
    destination[destination_size - 1U] = '\0';
}

static void fc_cleanup_error_message(DWORD error_code, char* output, size_t output_size) {
    DWORD length;

    if (output == NULL || output_size == 0U) {
        return;
    }

    output[0] = '\0';
    length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            error_code,
                            0,
                            output,
                            (DWORD) output_size,
                            NULL);
    if (length == 0U) {
        snprintf(output, output_size, "operation failed (error %lu)", (unsigned long) error_code);
        return;
    }

    while (length > 0U && (output[length - 1U] == '\r' || output[length - 1U] == '\n' || output[length - 1U] == ' ')) {
        output[length - 1U] = '\0';
        --length;
    }
}

void fc_cleanup_result_init(fc_cleanup_result* result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
}

void fc_cleanup_result_free(fc_cleanup_result* result) {
    if (result == NULL) {
        return;
    }

    free(result->items);
    memset(result, 0, sizeof(*result));
}

int fc_cleanup_execute(const fc_file_record* records,
                       size_t record_count,
                       const fc_cleanup_options* options,
                       fc_cleanup_result* result) {
    size_t index;

    if (records == NULL || result == NULL) {
        return 0;
    }

    for (index = 0; index < record_count; ++index) {
        const fc_file_record* record = &records[index];
        fc_cleanup_entry* entry;

        if (!fc_cleanup_reserve(result, result->item_count + 1U)) {
            return 0;
        }

        entry = &result->items[result->item_count++];
        memset(entry, 0, sizeof(*entry));
        fc_cleanup_copy(entry->path, sizeof(entry->path), record->path);
        entry->category = record->category;
        entry->size_bytes = record->size_bytes;

        ++result->planned_count;
        result->candidate_bytes += record->size_bytes;

        if (fc_cleanup_is_protected(record->path, options)) {
            fc_cleanup_copy(entry->message, sizeof(entry->message), "skipped: protected path");
            ++result->skipped_count;
            continue;
        }

        if (options != NULL && options->dry_run) {
            entry->simulated = 1;
            fc_cleanup_copy(entry->message, sizeof(entry->message), "dry-run: eligible for deletion");
            continue;
        }

        if (DeleteFileA(record->path) != 0) {
            entry->deleted = 1;
            fc_cleanup_copy(entry->message, sizeof(entry->message), "deleted");
            ++result->deleted_count;
            result->freed_bytes += record->size_bytes;
            continue;
        }

        fc_cleanup_error_message(GetLastError(), entry->message, sizeof(entry->message));
        ++result->skipped_count;
    }

    return 1;
}
