#include "core/path_filters.h"

#include <ctype.h>
#include <string.h>
#include <time.h>

static size_t fc_ascii_strlen(const char* value) {
    return value == NULL ? 0U : strlen(value);
}

int fc_path_normalize_copy(const char* input, char* output, size_t output_size) {
    size_t input_index = 0;
    size_t output_index = 0;
    int last_was_separator = 0;

    if (input == NULL || output == NULL || output_size == 0U) {
        return 0;
    }

    while (input[input_index] != '\0') {
        char current = input[input_index];
        if (current == '/') {
            current = '\\';
        } else {
            current = (char) tolower((unsigned char) current);
        }

        if (current == '\\') {
            if (last_was_separator) {
                ++input_index;
                continue;
            }
            last_was_separator = 1;
        } else {
            last_was_separator = 0;
        }

        if (output_index + 1U >= output_size) {
            return 0;
        }

        output[output_index++] = current;
        ++input_index;
    }

    while (output_index > 3U && output[output_index - 1U] == '\\') {
        --output_index;
    }

    output[output_index] = '\0';
    return 1;
}

int fc_path_is_under_root(const char* path, const char* root) {
    char normalized_path[FC_MAX_PATH_UTF8];
    char normalized_root[FC_MAX_PATH_UTF8];
    size_t root_length;

    if (!fc_path_normalize_copy(path, normalized_path, sizeof(normalized_path)) ||
        !fc_path_normalize_copy(root, normalized_root, sizeof(normalized_root))) {
        return 0;
    }

    root_length = strlen(normalized_root);
    if (root_length == 0U) {
        return 0;
    }

    if (strncmp(normalized_path, normalized_root, root_length) != 0) {
        return 0;
    }

    if (normalized_path[root_length] == '\0') {
        return 1;
    }

    if (normalized_root[root_length - 1U] == '\\') {
        return 1;
    }

    return normalized_path[root_length] == '\\';
}

int fc_path_is_protected(const char* path) {
    static const char* protected_roots[] = {
        "c:\\windows\\system32",
        "c:\\windows\\winsxs",
        "c:\\windows\\servicing",
        "c:\\windows\\boot",
        "c:\\program files",
        "c:\\program files (x86)",
        "c:\\documents and settings\\all users\\documents",
        "c:\\users\\public\\documents"
    };
    size_t index;

    if (path == NULL) {
        return 0;
    }

    for (index = 0; index < sizeof(protected_roots) / sizeof(protected_roots[0]); ++index) {
        if (fc_path_is_under_root(path, protected_roots[index])) {
            return 1;
        }
    }

    return 0;
}

int fc_path_matches_suffix(const char* path, const char* suffix) {
    char normalized_path[FC_MAX_PATH_UTF8];
    char normalized_suffix[FC_MAX_REASON_LEN];
    size_t path_length;
    size_t suffix_length;

    if (!fc_path_normalize_copy(path, normalized_path, sizeof(normalized_path)) ||
        !fc_path_normalize_copy(suffix, normalized_suffix, sizeof(normalized_suffix))) {
        return 0;
    }

    path_length = strlen(normalized_path);
    suffix_length = strlen(normalized_suffix);
    if (suffix_length == 0U || path_length < suffix_length) {
        return 0;
    }

    return strncmp(normalized_path + path_length - suffix_length, normalized_suffix, suffix_length) == 0;
}

int fc_path_matches_any_suffix(const char* path, const char** suffixes, size_t suffix_count) {
    size_t index;
    for (index = 0; index < suffix_count; ++index) {
        if (suffixes[index] != NULL && fc_path_matches_suffix(path, suffixes[index])) {
            return 1;
        }
    }
    return 0;
}

int fc_path_matches_any_token(const char* value, const char** tokens, size_t token_count) {
    char normalized_value[FC_MAX_PATH_UTF8];
    size_t index;

    if (!fc_path_normalize_copy(value, normalized_value, sizeof(normalized_value))) {
        return 0;
    }

    for (index = 0; index < token_count; ++index) {
        char normalized_token[FC_MAX_REASON_LEN];
        size_t token_length;
        size_t value_length;
        size_t offset;

        if (tokens[index] == NULL ||
            !fc_path_normalize_copy(tokens[index], normalized_token, sizeof(normalized_token))) {
            continue;
        }

        token_length = fc_ascii_strlen(normalized_token);
        value_length = fc_ascii_strlen(normalized_value);
        if (token_length == 0U || token_length > value_length) {
            continue;
        }

        for (offset = 0; offset + token_length <= value_length; ++offset) {
            if (strncmp(normalized_value + offset, normalized_token, token_length) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

int fc_record_matches_target(const char* path, const fc_scan_target* target, uint64_t last_write_unix) {
    uint64_t age_limit;
    uint64_t now_unix;

    if (path == NULL || target == NULL || target->root_path == NULL) {
        return 0;
    }

    if (!fc_path_is_under_root(path, target->root_path)) {
        return 0;
    }

    if (target->modified_within_days > 0U) {
        now_unix = (uint64_t) time(NULL);
        age_limit = (uint64_t) target->modified_within_days * 24ULL * 60ULL * 60ULL;
        if (last_write_unix == 0ULL || last_write_unix + age_limit < now_unix) {
            return 0;
        }
    }

    if (target->include_all_files) {
        return 1;
    }

    if (target->suffix_count > 0U &&
        fc_path_matches_any_suffix(path, target->suffixes, target->suffix_count)) {
        return 1;
    }

    if (target->name_contains_count > 0U &&
        fc_path_matches_any_token(path, target->name_contains, target->name_contains_count)) {
        return 1;
    }

    return 0;
}
