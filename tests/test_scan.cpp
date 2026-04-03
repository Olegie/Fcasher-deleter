#include "core/scan_engine.h"
#include "core/scan_result.h"
#include "test_common.hpp"

FC_TEST(scan_engine_finds_expected_temp_files) {
    const std::string root = fcasher::tests::makeTempDirectory("scan");
    const std::string nested = fcasher::tests::joinPath(root, "nested");

    FC_ASSERT(fcasher::tests::ensureDirectory(nested));
    FC_ASSERT(fcasher::tests::writeFile(fcasher::tests::joinPath(root, "hit.tmp"), "1234"));
    FC_ASSERT(fcasher::tests::writeFile(fcasher::tests::joinPath(root, "skip.bin"), "1234"));
    FC_ASSERT(fcasher::tests::writeFile(fcasher::tests::joinPath(nested, "nested.tmp"), "1234"));

    const char* suffixes[] = {".tmp"};
    fc_scan_target target {};
    target.root_path = root.c_str();
    target.category = FC_CATEGORY_USER_TEMP;
    target.removable_reason = "test temp";
    target.suffixes = suffixes;
    target.suffix_count = 1;
    target.recursive = 1;
    target.include_all_files = 0;

    fc_scan_result result {};
    fc_scan_result_init(&result);

    fc_scan_options options {};
    FC_ASSERT(fc_scan_targets(&target, 1, &options, &result) == 1);
    FC_ASSERT(result.file_count == 2);
    FC_ASSERT(result.scanned_directory_count >= 2);

    fc_scan_result_free(&result);
    fcasher::tests::removeTree(root);
    return true;
}
