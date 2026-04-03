#include <cstring>

#include "app/safety_policy.hpp"
#include "core/file_record.h"
#include "platform/windows_paths.hpp"
#include "test_common.hpp"

FC_TEST(safety_policy_blocks_protected_paths) {
    fcasher::platform::WindowsPaths paths;
    fcasher::app::SafetyPolicy policy(paths);

    fc_file_record record {};
    fc_file_record_init(&record);
    record.category = FC_CATEGORY_WINDOWS_TEMP;
    std::strncpy(record.path, (paths.windowsDir() + "\\System32\\kernel32.dll").c_str(), sizeof(record.path) - 1);

    std::string reason;
    FC_ASSERT(!policy.isDeletionAllowed(record, &reason));
    FC_ASSERT(reason == "protected root");
    return true;
}

FC_TEST(safety_policy_allows_temp_candidate) {
    fcasher::platform::WindowsPaths paths;
    fcasher::app::SafetyPolicy policy(paths);

    fc_file_record record {};
    fc_file_record_init(&record);
    record.category = FC_CATEGORY_USER_TEMP;
    std::strncpy(record.path, (paths.userTemp() + "\\unit.tmp").c_str(), sizeof(record.path) - 1);

    std::string reason;
    FC_ASSERT(policy.isDeletionAllowed(record, &reason));
    return true;
}
