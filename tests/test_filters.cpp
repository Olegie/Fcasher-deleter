#include <ctime>

#include "core/path_filters.h"
#include "test_common.hpp"

FC_TEST(path_normalization_and_protection) {
    char normalized[FC_MAX_PATH_UTF8] = {};
    FC_ASSERT(fc_path_normalize_copy("C:/Windows/Temp/", normalized, sizeof(normalized)) == 1);
    FC_ASSERT(std::string(normalized) == "c:\\windows\\temp");
    FC_ASSERT(fc_path_is_under_root("C:\\Windows\\Temp\\foo.tmp", "C:\\Windows\\Temp") == 1);
    FC_ASSERT(fc_path_is_protected("C:\\Windows\\System32\\config\\SAM") == 1);
    return true;
}

FC_TEST(record_match_target) {
    const char* suffixes[] = {".tmp"};
    fc_scan_target target {};
    target.root_path = "C:\\Temp";
    target.category = FC_CATEGORY_USER_TEMP;
    target.removable_reason = "test";
    target.suffixes = suffixes;
    target.suffix_count = 1;
    target.recursive = 1;
    target.include_all_files = 0;

    const auto now = static_cast<uint64_t>(std::time(nullptr));
    FC_ASSERT(fc_record_matches_target("C:\\Temp\\hit.tmp", &target, now) == 1);
    FC_ASSERT(fc_record_matches_target("C:\\Temp\\keep.bin", &target, now) == 0);
    FC_ASSERT(fc_record_matches_target("D:\\Temp\\hit.tmp", &target, now) == 0);
    return true;
}
