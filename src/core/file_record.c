#include "core/file_record.h"

#include <ctype.h>
#include <string.h>

typedef struct fc_category_descriptor {
    fc_category category;
    const char* key;
    const char* display_name;
} fc_category_descriptor;

static const fc_category_descriptor k_fc_categories[] = {
    {FC_CATEGORY_USER_TEMP, "user-temp", "User Temp"},
    {FC_CATEGORY_LOCAL_APP_TEMP, "local-app-temp", "Local App Temp"},
    {FC_CATEGORY_WINDOWS_TEMP, "windows-temp", "Windows Temp"},
    {FC_CATEGORY_LOGS, "logs", "Logs"},
    {FC_CATEGORY_THUMBNAIL_CACHE, "thumbnail-cache", "Thumbnail Cache"},
    {FC_CATEGORY_SHADER_CACHE, "shader-cache", "Shader Cache"},
    {FC_CATEGORY_BROWSER_CACHE, "browser-cache", "Browser Cache"},
    {FC_CATEGORY_CRASH_DUMPS, "crash-dumps", "Crash Dumps"},
    {FC_CATEGORY_RECENT_ARTIFACTS, "recent-artifacts", "Recent Artifacts"},
    {FC_CATEGORY_RECYCLE_BIN, "recycle-bin", "Recycle Bin"},
    {FC_CATEGORY_UNKNOWN, "unknown", "Unknown"}
};

static int fc_ascii_case_equal(const char* left, const char* right) {
    size_t index = 0;
    if (left == NULL || right == NULL) {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        if (tolower((unsigned char) left[index]) != tolower((unsigned char) right[index])) {
            return 0;
        }
        ++index;
    }

    return left[index] == '\0' && right[index] == '\0';
}

void fc_file_record_init(fc_file_record* record) {
    if (record == NULL) {
        return;
    }

    memset(record, 0, sizeof(*record));
    record->category = FC_CATEGORY_UNKNOWN;
}

const char* fc_category_name(fc_category category) {
    size_t index;
    for (index = 0; index < sizeof(k_fc_categories) / sizeof(k_fc_categories[0]); ++index) {
        if (k_fc_categories[index].category == category) {
            return k_fc_categories[index].key;
        }
    }
    return "unknown";
}

const char* fc_category_display_name(fc_category category) {
    size_t index;
    for (index = 0; index < sizeof(k_fc_categories) / sizeof(k_fc_categories[0]); ++index) {
        if (k_fc_categories[index].category == category) {
            return k_fc_categories[index].display_name;
        }
    }
    return "Unknown";
}

int fc_category_from_name(const char* value, fc_category* out_category) {
    size_t index;
    if (value == NULL || out_category == NULL) {
        return 0;
    }

    for (index = 0; index < sizeof(k_fc_categories) / sizeof(k_fc_categories[0]); ++index) {
        if (fc_ascii_case_equal(value, k_fc_categories[index].key)) {
            *out_category = k_fc_categories[index].category;
            return 1;
        }
    }

    return 0;
}
