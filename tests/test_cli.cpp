#include "app/cli.hpp"
#include "test_common.hpp"

FC_TEST(parse_scan_arguments) {
    fcasher::app::CliParser parser;
    char command[] = "fcasher";
    char mode[] = "scan";
    char all[] = "--all";
    char verbose[] = "--verbose";
    char exportFlag[] = "--export";
    char exportPath[] = "reports\\scan.txt";
    char jsonFlag[] = "--json";
    char jsonPath[] = "reports\\scan.json";

    char* argv[] = {command, mode, all, verbose, exportFlag, exportPath, jsonFlag, jsonPath};
    const auto options = parser.parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv);

    FC_ASSERT(options.command == fcasher::app::CommandType::Scan);
    FC_ASSERT(options.selectAll);
    FC_ASSERT(options.verbose);
    FC_ASSERT(options.exportPath.has_value());
    FC_ASSERT(options.exportPath.value() == "reports\\scan.txt");
    FC_ASSERT(options.jsonPath.has_value());
    FC_ASSERT(options.errors.empty());
    return true;
}

FC_TEST(parse_clean_category_arguments) {
    fcasher::app::CliParser parser;
    char command[] = "fcasher";
    char mode[] = "clean";
    char categoryFlag[] = "--category";
    char categoryValue[] = "browser-cache";
    char dryRun[] = "--dry-run";
    char yes[] = "--yes";

    char* argv[] = {command, mode, categoryFlag, categoryValue, dryRun, yes};
    const auto options = parser.parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv);

    FC_ASSERT(options.command == fcasher::app::CommandType::Clean);
    FC_ASSERT(!options.selectAll);
    FC_ASSERT(options.categoryTokens.size() == 1);
    FC_ASSERT(options.categoryTokens.front() == "browser-cache");
    FC_ASSERT(options.dryRun);
    FC_ASSERT(options.assumeYes);
    FC_ASSERT(options.errors.empty());
    return true;
}

FC_TEST(parse_analyze_filter_arguments) {
    fcasher::app::CliParser parser;
    char command[] = "fcasher";
    char mode[] = "analyze";
    char presetFlag[] = "--preset";
    char presetValue[] = "diagnostics";
    char minSizeFlag[] = "--min-size";
    char minSizeValue[] = "8MB";
    char olderFlag[] = "--older-than-days";
    char olderValue[] = "14";
    char sortFlag[] = "--sort";
    char sortValue[] = "size";
    char limitFlag[] = "--limit";
    char limitValue[] = "20";

    char* argv[] = {
        command,
        mode,
        presetFlag,
        presetValue,
        minSizeFlag,
        minSizeValue,
        olderFlag,
        olderValue,
        sortFlag,
        sortValue,
        limitFlag,
        limitValue
    };
    const auto options = parser.parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv);

    FC_ASSERT(options.command == fcasher::app::CommandType::Analyze);
    FC_ASSERT(options.presetTokens.size() == 1);
    FC_ASSERT(options.presetTokens.front() == "diagnostics");
    FC_ASSERT(options.minSizeBytes.has_value());
    FC_ASSERT(options.minSizeBytes.value() == 8ULL * 1024ULL * 1024ULL);
    FC_ASSERT(options.olderThanDays.has_value());
    FC_ASSERT(options.olderThanDays.value() == 14U);
    FC_ASSERT(options.limit.has_value());
    FC_ASSERT(options.limit.value() == 20U);
    FC_ASSERT(options.sortMode == fcasher::app::SortMode::SizeDesc);
    FC_ASSERT(options.errors.empty());
    return true;
}

FC_TEST(parse_categories_command_arguments) {
    fcasher::app::CliParser parser;
    char command[] = "fcasher";
    char mode[] = "categories";
    char exportFlag[] = "--export";
    char exportValue[] = "reports\\catalog.txt";

    char* argv[] = {command, mode, exportFlag, exportValue};
    const auto options = parser.parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv);

    FC_ASSERT(options.command == fcasher::app::CommandType::Categories);
    FC_ASSERT(options.exportPath.has_value());
    FC_ASSERT(options.exportPath.value() == "reports\\catalog.txt");
    FC_ASSERT(options.errors.empty());
    return true;
}
