#include "services/report_service.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "app/report_formatter.hpp"
#include "core/file_record.h"

namespace fcasher::services {
namespace {

constexpr std::size_t kDisplayedScanDirectoryLimit = 30;
constexpr std::size_t kAnalysisRankingLimit = 12;

struct DirectoryAggregate {
    std::string path;
    std::size_t fileCount{0};
    uint64_t totalSizeBytes{0};
};

std::string joinValues(const std::vector<std::string>& values, std::string_view emptyValue) {
    if (values.empty()) {
        return std::string(emptyValue);
    }

    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            out << ", ";
        }
        out << values[index];
    }
    return out.str();
}

std::string selectedCategoryNames(const std::vector<const fcasher::app::CategoryDefinition*>& categories) {
    std::vector<std::string> names;
    names.reserve(categories.size());
    for (const auto* category : categories) {
        names.push_back(category->displayName);
    }
    return joinValues(names, "(none)");
}

std::string cleanupStatus(const fc_cleanup_entry& entry) {
    if (entry.deleted != 0) {
        return "deleted";
    }
    if (entry.simulated != 0) {
        return "dry-run";
    }
    return "skipped";
}

std::string numberedLabel(std::size_t index, std::string_view suffix) {
    std::ostringstream out;
    out << '#' << std::setw(2) << std::setfill('0') << (index + 1) << std::setfill(' ') << "  " << suffix;
    return out.str();
}

std::string parentDirectory(std::string_view path) {
    const std::size_t slash = path.find_last_of("\\/");
    if (slash == std::string::npos) {
        return std::string(path);
    }
    return std::string(path.substr(0, slash));
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void appendSessionSection(std::ostringstream& out,
                          const ReportContext& context,
                          const fc_scan_result& result,
                          bool includeDryRun) {
    using fcasher::app::ReportFormatter;

    out << ReportFormatter::section("Session");
    out << ReportFormatter::keyValue("Mode", context.mode);
    out << ReportFormatter::keyValue("Selected categories", selectedCategoryNames(context.categories));
    if (!context.presets.empty()) {
        out << ReportFormatter::keyValue("Applied presets", joinValues(context.presets, "(none)"));
    }
    out << ReportFormatter::keyValue("Scanned directories", std::to_string(result.scanned_directory_count));
    if (context.sourceFileCount != 0U &&
        (context.sourceFileCount != result.file_count || context.sourceSizeBytes != result.total_size_bytes)) {
        out << ReportFormatter::keyValue("Matched before filters",
                                         std::to_string(context.sourceFileCount) + " files, " +
                                             ReportFormatter::humanSize(context.sourceSizeBytes));
    }
    out << ReportFormatter::keyValue("Effective candidates",
                                     std::to_string(result.file_count) + " files, " +
                                         ReportFormatter::humanSize(result.total_size_bytes));
    out << ReportFormatter::keyValue("Skipped items", std::to_string(result.skipped_count));
    out << ReportFormatter::keyValue("Sort order", context.sortMode);
    if (!context.activeFilters.empty()) {
        out << ReportFormatter::keyValue("Active filters", joinValues(context.activeFilters, "(none)"));
    }
    if (includeDryRun && context.dryRun) {
        out << ReportFormatter::keyValue("Execution", "dry-run requested");
    }
    out << '\n';
}

void appendCategorySummary(std::ostringstream& out,
                           const ReportContext& context,
                           const fc_scan_result& result) {
    using fcasher::app::ReportFormatter;

    const std::vector<std::size_t> summaryWidths {24, 10, 14};
    out << ReportFormatter::section("Category Summary");
    out << ReportFormatter::tableBorder(summaryWidths) << '\n';
    out << ReportFormatter::tableRow({"Category", "Files", "Size"}, summaryWidths) << '\n';
    out << ReportFormatter::tableBorder(summaryWidths) << '\n';
    for (const auto* category : context.categories) {
        out << ReportFormatter::tableRow(
                   {
                       category->displayName,
                       std::to_string(fc_scan_result_count_for_category(&result, category->id)),
                       ReportFormatter::humanSize(fc_scan_result_size_for_category(&result, category->id))
                   },
                   summaryWidths)
            << '\n';
    }
    out << ReportFormatter::tableBorder(summaryWidths) << '\n';
    out << ReportFormatter::tableRow(
               {"Total", std::to_string(result.file_count), ReportFormatter::humanSize(result.total_size_bytes)},
               summaryWidths)
        << '\n';
    out << ReportFormatter::tableBorder(summaryWidths) << "\n\n";
}

void appendScannedDirectories(std::ostringstream& out, const fc_scan_result& result) {
    using fcasher::app::ReportFormatter;

    out << ReportFormatter::section("Scanned Directories");
    if (result.scanned_directory_count == 0) {
        out << "  (none)\n\n";
        return;
    }

    const std::size_t displayedCount =
        result.scanned_directory_count < kDisplayedScanDirectoryLimit ? result.scanned_directory_count
                                                                      : kDisplayedScanDirectoryLimit;
    for (std::size_t index = 0; index < displayedCount; ++index) {
        out << "  - " << result.scanned_directories[index] << '\n';
    }
    if (displayedCount < result.scanned_directory_count) {
        out << "  ... " << (result.scanned_directory_count - displayedCount)
            << " more traversed directories omitted from console output\n";
    }
    out << '\n';
}

void appendFileRecords(std::ostringstream& out,
                       const fc_scan_result& result,
                       std::string_view sectionTitle) {
    using fcasher::app::ReportFormatter;

    out << ReportFormatter::section(sectionTitle);
    if (result.file_count == 0) {
        out << "  (none)\n\n";
        return;
    }

    for (std::size_t index = 0; index < result.file_count; ++index) {
        const auto& record = result.files[index];
        out << numberedLabel(index, "[" + std::string(fc_category_display_name(record.category)) + "]") << '\n';
        out << ReportFormatter::keyValue("  Path", record.path, 14);
        out << ReportFormatter::keyValue("  Size", ReportFormatter::humanSize(record.size_bytes), 14);
        out << ReportFormatter::keyValue("  Last Write", ReportFormatter::utcTimestamp(record.last_write_unix), 14);
        out << ReportFormatter::keyValue("  Reason", record.reason, 14);
        out << '\n';
    }
}

void appendSkippedItems(std::ostringstream& out, const fc_scan_result& result) {
    using fcasher::app::ReportFormatter;

    out << ReportFormatter::section("Skipped Items");
    if (result.skipped_count == 0) {
        out << "  (none)\n";
        return;
    }

    for (std::size_t index = 0; index < result.skipped_count; ++index) {
        out << numberedLabel(index, "[skip]") << '\n';
        out << ReportFormatter::keyValue("  Path", result.skipped[index].path, 14);
        out << ReportFormatter::keyValue("  Reason", result.skipped[index].reason, 14);
        out << '\n';
    }
}

std::vector<const fc_file_record*> rankedBySize(const fc_scan_result& result) {
    std::vector<const fc_file_record*> files;
    files.reserve(result.file_count);
    for (std::size_t index = 0; index < result.file_count; ++index) {
        files.push_back(&result.files[index]);
    }

    std::sort(files.begin(), files.end(), [](const fc_file_record* left, const fc_file_record* right) {
        if (left->size_bytes != right->size_bytes) {
            return left->size_bytes > right->size_bytes;
        }
        return std::string(left->path) < std::string(right->path);
    });
    return files;
}

std::vector<const fc_file_record*> rankedByAge(const fc_scan_result& result) {
    std::vector<const fc_file_record*> files;
    files.reserve(result.file_count);
    for (std::size_t index = 0; index < result.file_count; ++index) {
        files.push_back(&result.files[index]);
    }

    std::sort(files.begin(), files.end(), [](const fc_file_record* left, const fc_file_record* right) {
        const uint64_t leftAge = left->last_write_unix == 0 ? std::numeric_limits<uint64_t>::max() : left->last_write_unix;
        const uint64_t rightAge =
            right->last_write_unix == 0 ? std::numeric_limits<uint64_t>::max() : right->last_write_unix;
        if (leftAge != rightAge) {
            return leftAge < rightAge;
        }
        return left->size_bytes > right->size_bytes;
    });
    return files;
}

std::vector<DirectoryAggregate> aggregateDirectories(const fc_scan_result& result) {
    std::unordered_map<std::string, std::size_t> indexByKey;
    std::vector<DirectoryAggregate> aggregates;

    for (std::size_t index = 0; index < result.file_count; ++index) {
        const std::string directory = parentDirectory(result.files[index].path);
        const std::string key = lowercase(directory);
        auto found = indexByKey.find(key);
        if (found == indexByKey.end()) {
            found = indexByKey.emplace(key, aggregates.size()).first;
            aggregates.push_back({directory, 0U, 0ULL});
        }

        DirectoryAggregate& aggregate = aggregates[found->second];
        ++aggregate.fileCount;
        aggregate.totalSizeBytes += result.files[index].size_bytes;
    }

    std::sort(aggregates.begin(), aggregates.end(), [](const DirectoryAggregate& left, const DirectoryAggregate& right) {
        if (left.totalSizeBytes != right.totalSizeBytes) {
            return left.totalSizeBytes > right.totalSizeBytes;
        }
        if (left.fileCount != right.fileCount) {
            return left.fileCount > right.fileCount;
        }
        return left.path < right.path;
    });
    return aggregates;
}

void appendAnalysisRanking(std::ostringstream& out,
                           std::string_view title,
                           const std::vector<const fc_file_record*>& rankedFiles) {
    using fcasher::app::ReportFormatter;

    const std::vector<std::size_t> widths {6, 18, 12, 17, 40};
    out << ReportFormatter::section(title);
    if (rankedFiles.empty()) {
        out << "  (none)\n\n";
        return;
    }

    out << ReportFormatter::tableBorder(widths) << '\n';
    out << ReportFormatter::tableRow({"Rank", "Category", "Size", "Last Write", "Path"}, widths) << '\n';
    out << ReportFormatter::tableBorder(widths) << '\n';
    const std::size_t count = rankedFiles.size() < kAnalysisRankingLimit ? rankedFiles.size() : kAnalysisRankingLimit;
    for (std::size_t index = 0; index < count; ++index) {
        const auto* record = rankedFiles[index];
        out << ReportFormatter::tableRow(
                   {
                       std::to_string(index + 1),
                       fc_category_display_name(record->category),
                       ReportFormatter::humanSize(record->size_bytes),
                       ReportFormatter::utcTimestamp(record->last_write_unix),
                       record->path
                   },
                   widths)
            << '\n';
    }
    out << ReportFormatter::tableBorder(widths) << "\n\n";
}

void appendDirectoryHotspots(std::ostringstream& out, const std::vector<DirectoryAggregate>& aggregates) {
    using fcasher::app::ReportFormatter;

    const std::vector<std::size_t> widths {8, 12, 12, 56};
    out << ReportFormatter::section("Directory Hot Spots");
    if (aggregates.empty()) {
        out << "  (none)\n\n";
        return;
    }

    out << ReportFormatter::tableBorder(widths) << '\n';
    out << ReportFormatter::tableRow({"Rank", "Files", "Size", "Directory"}, widths) << '\n';
    out << ReportFormatter::tableBorder(widths) << '\n';
    const std::size_t count = aggregates.size() < kAnalysisRankingLimit ? aggregates.size() : kAnalysisRankingLimit;
    for (std::size_t index = 0; index < count; ++index) {
        out << ReportFormatter::tableRow(
                   {
                       std::to_string(index + 1),
                       std::to_string(aggregates[index].fileCount),
                       ReportFormatter::humanSize(aggregates[index].totalSizeBytes),
                       aggregates[index].path
                   },
                   widths)
            << '\n';
    }
    out << ReportFormatter::tableBorder(widths) << "\n\n";
}

std::string yesNo(bool value) {
    return value ? "yes" : "no";
}

std::string ruleSummary(const fcasher::app::CategoryDefinition& category) {
    std::vector<std::string> rules;
    if (!category.suffixes.empty()) {
        rules.push_back("suffix filter");
    }
    if (!category.containsTokens.empty()) {
        rules.push_back("name token filter");
    }
    if (category.includeAllFiles) {
        rules.push_back("all files in root");
    }
    if (category.modifiedWithinDays != 0U) {
        rules.push_back("recent <= " + std::to_string(category.modifiedWithinDays) + " days");
    }
    if (rules.empty()) {
        rules.push_back("root-only policy");
    }
    return joinValues(rules, "root-only policy");
}

}  // namespace

std::string ReportService::buildScanReport(const ReportContext& context,
                                           const fc_scan_result& result,
                                           bool previewStyle) const {
    using fcasher::app::ReportFormatter;

    std::ostringstream out;
    out << ReportFormatter::logo() << '\n';
    out << ReportFormatter::banner(previewStyle ? "FCASHER :: PREVIEW REPORT" : "FCASHER :: SCAN REPORT",
                                   "Inspect first. Review exact paths. Delete only with intent.")
        << "\n\n";

    appendSessionSection(out, context, result, true);
    appendCategorySummary(out, context, result);
    appendScannedDirectories(out, result);
    appendFileRecords(out, result, previewStyle ? "Files Selected for Cleanup" : "Detected Removable Files");
    appendSkippedItems(out, result);
    return out.str();
}

std::string ReportService::buildCleanupReport(const ReportContext& context,
                                              const fc_cleanup_result& cleanup) const {
    using fcasher::app::ReportFormatter;

    const std::vector<std::size_t> outcomeWidths {10, 16, 12, 49};
    std::ostringstream out;

    out << ReportFormatter::banner("FCASHER :: CLEANUP REPORT",
                                   "Controlled deletion results with per-item status and reclaimed space.")
        << "\n\n";

    out << ReportFormatter::section("Session");
    out << ReportFormatter::keyValue("Mode", context.mode);
    out << ReportFormatter::keyValue("Selected categories", selectedCategoryNames(context.categories));
    if (!context.presets.empty()) {
        out << ReportFormatter::keyValue("Applied presets", joinValues(context.presets, "(none)"));
    }
    out << ReportFormatter::keyValue("Requested cleanup items", std::to_string(cleanup.planned_count));
    out << ReportFormatter::keyValue("Deleted items", std::to_string(cleanup.deleted_count));
    out << ReportFormatter::keyValue("Skipped items", std::to_string(cleanup.skipped_count));
    out << ReportFormatter::keyValue("Potential reclaimable size", ReportFormatter::humanSize(cleanup.candidate_bytes));
    out << ReportFormatter::keyValue("Freed size", ReportFormatter::humanSize(cleanup.freed_bytes));
    out << ReportFormatter::keyValue("Sort order", context.sortMode);
    if (!context.activeFilters.empty()) {
        out << ReportFormatter::keyValue("Active filters", joinValues(context.activeFilters, "(none)"));
    }
    out << '\n';

    out << ReportFormatter::section("Cleanup Outcomes");
    if (cleanup.item_count == 0) {
        out << "  (none)\n";
    } else {
        out << ReportFormatter::tableBorder(outcomeWidths) << '\n';
        out << ReportFormatter::tableRow({"Status", "Category", "Size", "Path"}, outcomeWidths) << '\n';
        out << ReportFormatter::tableBorder(outcomeWidths) << '\n';
        for (std::size_t index = 0; index < cleanup.item_count; ++index) {
            const auto& entry = cleanup.items[index];
            out << ReportFormatter::tableRow(
                       {
                           cleanupStatus(entry),
                           fc_category_display_name(entry.category),
                           ReportFormatter::humanSize(entry.size_bytes),
                           entry.path
                       },
                       outcomeWidths)
                << '\n';
            out << ReportFormatter::keyValue("  Detail", entry.message, 14);
        }
        out << ReportFormatter::tableBorder(outcomeWidths) << '\n';
    }

    return out.str();
}

std::string ReportService::buildAnalysisReport(const ReportContext& context,
                                               const fc_scan_result& result) const {
    using fcasher::app::ReportFormatter;

    std::ostringstream out;
    out << ReportFormatter::logo() << '\n';
    out << ReportFormatter::banner("FCASHER :: ANALYSIS REPORT",
                                   "Ranked cache intelligence for targeted cleanup decisions.")
        << "\n\n";

    appendSessionSection(out, context, result, false);
    appendCategorySummary(out, context, result);
    appendAnalysisRanking(out, "Largest Candidates", rankedBySize(result));
    appendAnalysisRanking(out, "Oldest Candidates", rankedByAge(result));
    appendDirectoryHotspots(out, aggregateDirectories(result));
    appendSkippedItems(out, result);
    return out.str();
}

std::string ReportService::buildCategoryCatalogReport(
    const std::vector<fcasher::app::CategoryDefinition>& categories,
    const std::vector<fcasher::app::PresetDefinition>& presets) const {
    using fcasher::app::ReportFormatter;

    const std::vector<std::size_t> categoryWidths {20, 11, 11, 46};
    const std::vector<std::size_t> presetWidths {18, 58};
    std::ostringstream out;

    out << ReportFormatter::logo() << '\n';
    out << ReportFormatter::banner("FCASHER :: CATEGORY CATALOG",
                                   "Built-in scan domains, roots, rules, and operator presets.")
        << "\n\n";

    out << ReportFormatter::section("Summary");
    out << ReportFormatter::keyValue("Built-in categories", std::to_string(categories.size()));
    out << ReportFormatter::keyValue("Built-in presets", std::to_string(presets.size()));
    out << '\n';

    out << ReportFormatter::section("Categories");
    out << ReportFormatter::tableBorder(categoryWidths) << '\n';
    out << ReportFormatter::tableRow({"Category", "In --all", "Explicit", "Description"}, categoryWidths) << '\n';
    out << ReportFormatter::tableBorder(categoryWidths) << '\n';
    for (const auto& category : categories) {
        out << ReportFormatter::tableRow(
                   {
                       category.displayName + " (" + category.key + ")",
                       yesNo(category.includedInAll),
                       yesNo(category.explicitOnly),
                       category.description
                   },
                   categoryWidths)
            << '\n';
    }
    out << ReportFormatter::tableBorder(categoryWidths) << "\n\n";

    out << ReportFormatter::section("Presets");
    out << ReportFormatter::tableBorder(presetWidths) << '\n';
    out << ReportFormatter::tableRow({"Preset", "Description"}, presetWidths) << '\n';
    out << ReportFormatter::tableBorder(presetWidths) << '\n';
    for (const auto& preset : presets) {
        out << ReportFormatter::tableRow({preset.displayName + " (" + preset.key + ")", preset.description}, presetWidths)
            << '\n';
    }
    out << ReportFormatter::tableBorder(presetWidths) << "\n\n";

    out << ReportFormatter::section("Category Details");
    for (const auto& category : categories) {
        out << category.displayName << " [" << category.key << "]\n";
        out << ReportFormatter::keyValue("  Description", category.description, 14);
        out << ReportFormatter::keyValue("  Removal rule", category.removableReason, 14);
        out << ReportFormatter::keyValue("  Match rules", ruleSummary(category), 14);
        out << ReportFormatter::keyValue("  Recursive", yesNo(category.recursive), 14);
        out << ReportFormatter::keyValue("  Included in --all", yesNo(category.includedInAll), 14);
        out << ReportFormatter::keyValue("  Explicit only", yesNo(category.explicitOnly), 14);
        if (category.roots.empty()) {
            out << "  Roots        : (none)\n\n";
            continue;
        }

        for (std::size_t index = 0; index < category.roots.size(); ++index) {
            out << ReportFormatter::keyValue(index == 0 ? "  Roots" : "  ", category.roots[index], 14);
        }
        out << '\n';
    }

    if (!presets.empty()) {
        out << ReportFormatter::section("Preset Details");
        for (const auto& preset : presets) {
            out << preset.displayName << " [" << preset.key << "]\n";
            out << ReportFormatter::keyValue("  Description", preset.description, 14);
            out << ReportFormatter::keyValue("  Expands to", joinValues(preset.categoryTokens, "(none)"), 14);
            out << '\n';
        }
    }

    return out.str();
}

}  // namespace fcasher::services
