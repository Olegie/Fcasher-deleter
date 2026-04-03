#include "app/command_dispatcher.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "app/report_formatter.hpp"
#include "core/cleanup_engine.h"
#include "core/scan_engine.h"
#include "core/scan_result.h"

namespace fcasher::app {
namespace {

std::string modeName(CommandType command) {
    switch (command) {
    case CommandType::Scan:
        return "scan";
    case CommandType::Preview:
        return "preview";
    case CommandType::Clean:
        return "clean";
    case CommandType::Report:
        return "report";
    case CommandType::Analyze:
        return "analyze";
    case CommandType::Categories:
        return "categories";
    case CommandType::Help:
    default:
        return "help";
    }
}

std::string sortModeName(SortMode mode) {
    switch (mode) {
    case SortMode::SizeDesc:
        return "size-desc";
    case SortMode::SizeAsc:
        return "size-asc";
    case SortMode::TimeNewest:
        return "time-newest";
    case SortMode::TimeOldest:
        return "time-oldest";
    case SortMode::PathAsc:
        return "path-asc";
    case SortMode::Category:
        return "category";
    case SortMode::Native:
    default:
        return "native";
    }
}

bool confirmCleanup() {
    std::cout << "Type YES to continue with file deletion: ";
    std::string confirmation;
    std::getline(std::cin, confirmation);
    return confirmation == "YES";
}

struct TargetBacking {
    std::vector<const char*> suffixes;
    std::vector<const char*> tokens;
};

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool caseInsensitiveLess(std::string_view left, std::string_view right) {
    return lowercase(std::string(left)) < lowercase(std::string(right));
}

std::vector<std::string> presetDisplayNames(const CategoryRegistry& registry,
                                            const std::vector<std::string>& presetTokens) {
    std::vector<std::string> names;
    for (const auto& presetToken : presetTokens) {
        if (const auto* preset = registry.findPresetByKey(presetToken)) {
            names.push_back(preset->displayName);
        } else {
            names.push_back(presetToken);
        }
    }
    return names;
}

std::vector<std::string> describeFilters(const CliOptions& options) {
    std::vector<std::string> filters;
    if (options.minSizeBytes.has_value()) {
        filters.push_back("min-size >= " + ReportFormatter::humanSize(*options.minSizeBytes));
    }
    if (options.maxSizeBytes.has_value()) {
        filters.push_back("max-size <= " + ReportFormatter::humanSize(*options.maxSizeBytes));
    }
    if (options.modifiedWithinDays.has_value()) {
        filters.push_back("modified within " + std::to_string(*options.modifiedWithinDays) + " day(s)");
    }
    if (options.olderThanDays.has_value()) {
        filters.push_back("older than " + std::to_string(*options.olderThanDays) + " day(s)");
    }
    if (options.limit.has_value()) {
        filters.push_back("limit " + std::to_string(*options.limit));
    }
    return filters;
}

bool cloneScanMetadata(const fc_scan_result& source, fc_scan_result* destination) {
    for (std::size_t index = 0; index < source.scanned_directory_count; ++index) {
        if (!fc_scan_result_add_scanned_directory(destination, source.scanned_directories[index])) {
            return false;
        }
    }

    for (std::size_t index = 0; index < source.skipped_count; ++index) {
        if (!fc_scan_result_add_skipped(destination, source.skipped[index].path, source.skipped[index].reason)) {
            return false;
        }
    }

    return true;
}

bool matchesTimeFilters(const fc_file_record& record, const CliOptions& options, uint64_t nowUnix) {
    if (!options.modifiedWithinDays.has_value() && !options.olderThanDays.has_value()) {
        return true;
    }

    if (record.last_write_unix == 0U) {
        return false;
    }

    if (options.modifiedWithinDays.has_value()) {
        const uint64_t maxAgeSeconds = static_cast<uint64_t>(*options.modifiedWithinDays) * 86400ULL;
        if (record.last_write_unix + maxAgeSeconds < nowUnix) {
            return false;
        }
    }

    if (options.olderThanDays.has_value()) {
        const uint64_t minAgeSeconds = static_cast<uint64_t>(*options.olderThanDays) * 86400ULL;
        if (record.last_write_unix + minAgeSeconds > nowUnix) {
            return false;
        }
    }

    return true;
}

bool matchesCliFilters(const fc_file_record& record, const CliOptions& options, uint64_t nowUnix) {
    if (options.minSizeBytes.has_value() && record.size_bytes < *options.minSizeBytes) {
        return false;
    }
    if (options.maxSizeBytes.has_value() && record.size_bytes > *options.maxSizeBytes) {
        return false;
    }
    return matchesTimeFilters(record, options, nowUnix);
}

bool compareRecords(const fc_file_record* left, const fc_file_record* right, SortMode mode) {
    switch (mode) {
    case SortMode::SizeDesc:
        if (left->size_bytes != right->size_bytes) {
            return left->size_bytes > right->size_bytes;
        }
        return caseInsensitiveLess(left->path, right->path);
    case SortMode::SizeAsc:
        if (left->size_bytes != right->size_bytes) {
            return left->size_bytes < right->size_bytes;
        }
        return caseInsensitiveLess(left->path, right->path);
    case SortMode::TimeNewest:
        if (left->last_write_unix != right->last_write_unix) {
            return left->last_write_unix > right->last_write_unix;
        }
        return left->size_bytes > right->size_bytes;
    case SortMode::TimeOldest:
        if (left->last_write_unix != right->last_write_unix) {
            return left->last_write_unix < right->last_write_unix;
        }
        return left->size_bytes > right->size_bytes;
    case SortMode::PathAsc:
        return caseInsensitiveLess(left->path, right->path);
    case SortMode::Category: {
        const std::string leftCategory = fc_category_display_name(left->category);
        const std::string rightCategory = fc_category_display_name(right->category);
        if (leftCategory != rightCategory) {
            return caseInsensitiveLess(leftCategory, rightCategory);
        }
        if (left->size_bytes != right->size_bytes) {
            return left->size_bytes > right->size_bytes;
        }
        return caseInsensitiveLess(left->path, right->path);
    }
    case SortMode::Native:
    default:
        return false;
    }
}

bool buildEffectiveResult(const fc_scan_result& source, const CliOptions& options, fc_scan_result* destination) {
    std::vector<const fc_file_record*> selectedFiles;
    selectedFiles.reserve(source.file_count);
    const uint64_t nowUnix = static_cast<uint64_t>(std::time(nullptr));

    if (!cloneScanMetadata(source, destination)) {
        return false;
    }

    for (std::size_t index = 0; index < source.file_count; ++index) {
        if (matchesCliFilters(source.files[index], options, nowUnix)) {
            selectedFiles.push_back(&source.files[index]);
        }
    }

    if (options.sortMode != SortMode::Native) {
        std::sort(selectedFiles.begin(),
                  selectedFiles.end(),
                  [&](const fc_file_record* left, const fc_file_record* right) {
                      return compareRecords(left, right, options.sortMode);
                  });
    }

    const std::size_t limit =
        options.limit.has_value() && *options.limit < selectedFiles.size() ? *options.limit : selectedFiles.size();
    for (std::size_t index = 0; index < limit; ++index) {
        if (!fc_scan_result_add_file(destination, selectedFiles[index])) {
            return false;
        }
    }

    return true;
}

}  // namespace

CommandDispatcher::CommandDispatcher()
    : paths_(),
      registry_(paths_),
      safety_(paths_),
      reportService_(),
      exportService_() {}

int CommandDispatcher::run(const CliOptions& options) {
    if (options.command == CommandType::Categories) {
        const std::string report = reportService_.buildCategoryCatalogReport(registry_.all(), registry_.presets());
        std::cout << report;

        if (options.exportPath.has_value()) {
            std::string error;
            if (!exportService_.writeText(*options.exportPath, report, error)) {
                std::cerr << error << '\n';
                return 1;
            }
            if (options.verbose) {
                std::cout << "Text report written to: " << *options.exportPath << '\n';
            }
        }

        if (options.jsonPath.has_value()) {
            std::cerr << "warning: --json is not supported for the categories command.\n";
        }

        return 0;
    }

    const bool selectAll =
        options.selectAll ||
        ((options.command == CommandType::Report || options.command == CommandType::Analyze) && !options.hasSelection());
    std::vector<std::string> warnings;
    std::vector<std::string> categoryTokens = options.categoryTokens;
    const auto presetCategories = registry_.expandPresetTokens(options.presetTokens, &warnings);
    categoryTokens.insert(categoryTokens.end(), presetCategories.begin(), presetCategories.end());

    const auto categories = registry_.resolve(categoryTokens, selectAll, &warnings);

    for (const auto& warning : warnings) {
        std::cerr << "warning: " << warning << '\n';
    }

    if (categories.empty()) {
        std::cerr << "No supported cleanup categories were selected.\n";
        return 1;
    }

    std::size_t totalRoots = 0;
    for (const auto* category : categories) {
        totalRoots += category->roots.size();
    }

    std::vector<TargetBacking> backings;
    std::vector<fc_scan_target> targets;
    backings.reserve(totalRoots);
    targets.reserve(totalRoots);

    for (const auto* category : categories) {
        for (const auto& root : category->roots) {
            if (root.empty()) {
                continue;
            }

            backings.push_back({});
            for (const auto& suffix : category->suffixes) {
                backings.back().suffixes.push_back(suffix.c_str());
            }
            for (const auto& token : category->containsTokens) {
                backings.back().tokens.push_back(token.c_str());
            }

            fc_scan_target target {};
            target.root_path = root.c_str();
            target.category = category->id;
            target.removable_reason = category->removableReason.c_str();
            target.suffixes = backings.back().suffixes.empty() ? nullptr : backings.back().suffixes.data();
            target.suffix_count = backings.back().suffixes.size();
            target.name_contains = backings.back().tokens.empty() ? nullptr : backings.back().tokens.data();
            target.name_contains_count = backings.back().tokens.size();
            target.modified_within_days = category->modifiedWithinDays;
            target.recursive = category->recursive ? 1 : 0;
            target.include_all_files = category->includeAllFiles ? 1 : 0;
            targets.push_back(target);
        }
    }

    if (targets.empty()) {
        std::cerr << "The selected categories do not have any configured scan roots.\n";
        return 1;
    }

    std::vector<const char*> protectedRoots;
    protectedRoots.reserve(safety_.protectedRoots().size());
    for (const auto& root : safety_.protectedRoots()) {
        protectedRoots.push_back(root.c_str());
    }

    fc_scan_result scanResult {};
    fc_scan_result_init(&scanResult);

    fc_scan_options scanOptions {};
    scanOptions.protected_roots = protectedRoots.data();
    scanOptions.protected_root_count = protectedRoots.size();
    scanOptions.verbose = options.verbose ? 1 : 0;

    if (!fc_scan_targets(targets.data(), targets.size(), &scanOptions, &scanResult)) {
        fc_scan_result_free(&scanResult);
        std::cerr << "Scan failed because the low-level scan engine returned an error.\n";
        return 1;
    }

    fc_scan_result effectiveResult {};
    fc_scan_result_init(&effectiveResult);
    if (!buildEffectiveResult(scanResult, options, &effectiveResult)) {
        fc_scan_result_free(&effectiveResult);
        fc_scan_result_free(&scanResult);
        std::cerr << "Filtering failed while preparing the effective result set.\n";
        return 1;
    }

    services::ReportContext reportContext {
        modeName(options.command),
        categories,
        presetDisplayNames(registry_, options.presetTokens),
        describeFilters(options),
        sortModeName(options.sortMode),
        scanResult.file_count,
        scanResult.total_size_bytes,
        options.dryRun,
        options.verbose
    };

    const bool previewStyle = options.command == CommandType::Preview || options.command == CommandType::Clean;
    const std::string scanReport =
        options.command == CommandType::Analyze ? reportService_.buildAnalysisReport(reportContext, effectiveResult)
                                                : reportService_.buildScanReport(reportContext, effectiveResult, previewStyle);
    std::cout << scanReport;

    if (options.jsonPath.has_value()) {
        std::string error;
        if (!exportService_.writeScanJson(*options.jsonPath, effectiveResult, categories, error)) {
            std::cerr << error << '\n';
        } else if (options.verbose) {
            std::cout << "JSON export written to: " << *options.jsonPath << '\n';
        }
    }

    if (options.command == CommandType::Scan ||
        options.command == CommandType::Preview ||
        options.command == CommandType::Report ||
        options.command == CommandType::Analyze) {
        if (options.exportPath.has_value()) {
            std::string error;
            if (!exportService_.writeText(*options.exportPath, scanReport, error)) {
                std::cerr << error << '\n';
                fc_scan_result_free(&effectiveResult);
                fc_scan_result_free(&scanResult);
                return 1;
            }
            if (options.verbose) {
                std::cout << "Text report written to: " << *options.exportPath << '\n';
            }
        }

        fc_scan_result_free(&effectiveResult);
        fc_scan_result_free(&scanResult);
        return 0;
    }

    if (effectiveResult.file_count == 0) {
        if (options.exportPath.has_value()) {
            std::string error;
            if (!exportService_.writeText(*options.exportPath, scanReport, error)) {
                std::cerr << error << '\n';
            }
        }
        fc_scan_result_free(&effectiveResult);
        fc_scan_result_free(&scanResult);
        return 0;
    }

    if (!options.dryRun && !options.assumeYes && !confirmCleanup()) {
        fc_scan_result_free(&effectiveResult);
        fc_scan_result_free(&scanResult);
        std::cout << "Cleanup cancelled.\n";
        return 2;
    }

    fc_cleanup_result cleanupResult {};
    fc_cleanup_result_init(&cleanupResult);

    fc_cleanup_options cleanupOptions {};
    cleanupOptions.dry_run = options.dryRun ? 1 : 0;
    cleanupOptions.verbose = options.verbose ? 1 : 0;
    cleanupOptions.protected_roots = protectedRoots.data();
    cleanupOptions.protected_root_count = protectedRoots.size();

    if (!fc_cleanup_execute(effectiveResult.files, effectiveResult.file_count, &cleanupOptions, &cleanupResult)) {
        fc_cleanup_result_free(&cleanupResult);
        fc_scan_result_free(&effectiveResult);
        fc_scan_result_free(&scanResult);
        std::cerr << "Cleanup failed because the low-level cleanup engine returned an error.\n";
        return 1;
    }

    const std::string cleanupReport = reportService_.buildCleanupReport(reportContext, cleanupResult);
    std::cout << '\n' << cleanupReport;

    if (options.exportPath.has_value()) {
        std::string error;
        if (!exportService_.writeText(*options.exportPath, cleanupReport, error)) {
            std::cerr << error << '\n';
            fc_cleanup_result_free(&cleanupResult);
            fc_scan_result_free(&effectiveResult);
            fc_scan_result_free(&scanResult);
            return 1;
        }
        if (options.verbose) {
            std::cout << "Text report written to: " << *options.exportPath << '\n';
        }
    }

    fc_cleanup_result_free(&cleanupResult);
    fc_scan_result_free(&effectiveResult);
    fc_scan_result_free(&scanResult);
    return 0;
}

}  // namespace fcasher::app
