#include "core/scan_engine.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

static uint64_t fc_filetime_to_unix_seconds(const FILETIME* filetime) {
    ULARGE_INTEGER value;
    const uint64_t k_windows_to_unix_epoch = 116444736000000000ULL;

    value.LowPart = filetime->dwLowDateTime;
    value.HighPart = filetime->dwHighDateTime;
    if (value.QuadPart <= k_windows_to_unix_epoch) {
        return 0ULL;
    }
    return (value.QuadPart - k_windows_to_unix_epoch) / 10000000ULL;
}

static void fc_copy_message(char* destination, size_t destination_size, const char* value) {
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

static void fc_format_windows_error(DWORD error_code,
                                    const char* prefix,
                                    char* output,
                                    size_t output_size) {
    char system_message[128];
    DWORD length;

    if (output == NULL || output_size == 0U) {
        return;
    }

    output[0] = '\0';
    memset(system_message, 0, sizeof(system_message));
    length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            error_code,
                            0,
                            system_message,
                            (DWORD) sizeof(system_message),
                            NULL);

    if (length == 0U) {
        snprintf(output, output_size, "%s (error %lu)", prefix, (unsigned long) error_code);
        output[output_size - 1U] = '\0';
        return;
    }

    while (length > 0U &&
           (system_message[length - 1U] == '\r' || system_message[length - 1U] == '\n' ||
            system_message[length - 1U] == ' ')) {
        system_message[length - 1U] = '\0';
        --length;
    }

    snprintf(output, output_size, "%s: %s", prefix, system_message);
    output[output_size - 1U] = '\0';
}

static void fc_join_path(const char* directory, const char* name, char* output, size_t output_size) {
    if (directory == NULL || name == NULL || output == NULL || output_size == 0U) {
        return;
    }

    if (directory[0] == '\0') {
        fc_copy_message(output, output_size, name);
        return;
    }

    if (directory[strlen(directory) - 1U] == '\\') {
        snprintf(output, output_size, "%s%s", directory, name);
    } else {
        snprintf(output, output_size, "%s\\%s", directory, name);
    }
}

static int fc_is_protected_by_options(const char* path, const fc_scan_options* options) {
    size_t index;

    if (path == NULL || options == NULL) {
        return 0;
    }

    for (index = 0; index < options->protected_root_count; ++index) {
        const char* protected_root = options->protected_roots[index];
        if (protected_root != NULL && fc_path_is_under_root(path, protected_root)) {
            return 1;
        }
    }

    return 0;
}

static int fc_scan_directory(const fc_scan_target* target,
                             const fc_scan_options* options,
                             const char* directory,
                             fc_scan_result* result) {
    WIN32_FIND_DATAA find_data;
    HANDLE handle;
    char query[FC_MAX_PATH_UTF8];

    if (!fc_scan_result_add_scanned_directory(result, directory)) {
        return 0;
    }

    fc_join_path(directory, "*", query, sizeof(query));
    handle = FindFirstFileA(query, &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error_code = GetLastError();
        char reason[FC_MAX_REASON_LEN];
        fc_format_windows_error(error_code, "directory unavailable", reason, sizeof(reason));
        return fc_scan_result_add_skipped(result, directory, reason);
    }

    do {
        char candidate_path[FC_MAX_PATH_UTF8];

        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        fc_join_path(directory, find_data.cFileName, candidate_path, sizeof(candidate_path));

        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
            if (!fc_scan_result_add_skipped(result, candidate_path, "reparse point skipped")) {
                FindClose(handle);
                return 0;
            }
            continue;
        }

        if (fc_path_is_protected(candidate_path) || fc_is_protected_by_options(candidate_path, options)) {
            if (!fc_scan_result_add_skipped(result, candidate_path, "protected path")) {
                FindClose(handle);
                return 0;
            }
            continue;
        }

        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
            if (target->recursive) {
                if (!fc_scan_directory(target, options, candidate_path, result)) {
                    FindClose(handle);
                    return 0;
                }
            }
            continue;
        }

        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0U) {
            if (!fc_scan_result_add_skipped(result, candidate_path, "system-marked file skipped")) {
                FindClose(handle);
                return 0;
            }
            continue;
        }

        if (fc_record_matches_target(candidate_path,
                                     target,
                                     fc_filetime_to_unix_seconds(&find_data.ftLastWriteTime))) {
            fc_file_record record;
            ULARGE_INTEGER size_value;

            fc_file_record_init(&record);
            fc_copy_message(record.path, sizeof(record.path), candidate_path);
            fc_copy_message(record.reason, sizeof(record.reason), target->removable_reason);
            record.category = target->category;
            record.last_write_unix = fc_filetime_to_unix_seconds(&find_data.ftLastWriteTime);
            size_value.LowPart = find_data.nFileSizeLow;
            size_value.HighPart = find_data.nFileSizeHigh;
            record.size_bytes = size_value.QuadPart;

            if (!fc_scan_result_add_file(result, &record)) {
                FindClose(handle);
                return 0;
            }
        }
    } while (FindNextFileA(handle, &find_data) != 0);

    FindClose(handle);
    return 1;
}

int fc_scan_targets(const fc_scan_target* targets,
                    size_t target_count,
                    const fc_scan_options* options,
                    fc_scan_result* result) {
    size_t index;

    if (targets == NULL || result == NULL) {
        return 0;
    }

    for (index = 0; index < target_count; ++index) {
        DWORD attributes;
        const char* root = targets[index].root_path;

        if (root == NULL || root[0] == '\0') {
            continue;
        }

        if (fc_path_is_protected(root) || fc_is_protected_by_options(root, options)) {
            if (!fc_scan_result_add_skipped(result, root, "protected scan root")) {
                return 0;
            }
            continue;
        }

        attributes = GetFileAttributesA(root);
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            char reason[FC_MAX_REASON_LEN];
            fc_format_windows_error(GetLastError(), "scan root unavailable", reason, sizeof(reason));
            if (!fc_scan_result_add_skipped(result, root, reason)) {
                return 0;
            }
            continue;
        }

        if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0U) {
            if (!fc_scan_result_add_skipped(result, root, "scan root is not a directory")) {
                return 0;
            }
            continue;
        }

        if (!fc_scan_directory(&targets[index], options, root, result)) {
            return 0;
        }
    }

    return 1;
}
