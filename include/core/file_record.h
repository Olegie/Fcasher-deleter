#ifndef FCASHER_CORE_FILE_RECORD_H
#define FCASHER_CORE_FILE_RECORD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FC_MAX_PATH_UTF8 2048
#define FC_MAX_REASON_LEN 160

typedef enum fc_category {
    FC_CATEGORY_USER_TEMP = 0,
    FC_CATEGORY_LOCAL_APP_TEMP,
    FC_CATEGORY_WINDOWS_TEMP,
    FC_CATEGORY_LOGS,
    FC_CATEGORY_THUMBNAIL_CACHE,
    FC_CATEGORY_SHADER_CACHE,
    FC_CATEGORY_BROWSER_CACHE,
    FC_CATEGORY_CRASH_DUMPS,
    FC_CATEGORY_RECENT_ARTIFACTS,
    FC_CATEGORY_RECYCLE_BIN,
    FC_CATEGORY_UNKNOWN
} fc_category;

typedef struct fc_file_record {
    char path[FC_MAX_PATH_UTF8];
    char reason[FC_MAX_REASON_LEN];
    fc_category category;
    uint64_t size_bytes;
    uint64_t last_write_unix;
} fc_file_record;

void fc_file_record_init(fc_file_record* record);
const char* fc_category_name(fc_category category);
const char* fc_category_display_name(fc_category category);
int fc_category_from_name(const char* value, fc_category* out_category);

#ifdef __cplusplus
}
#endif

#endif
