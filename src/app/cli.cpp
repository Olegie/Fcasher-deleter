#include "app/cli.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "app/report_formatter.hpp"

namespace fcasher::app {
namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trim(std::string value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

void addCategoryToken(CliOptions& options, const std::string& token) {
    options.categoryTokens.push_back(toLower(token));
}

void addPresetToken(CliOptions& options, const std::string& token) {
    options.presetTokens.push_back(toLower(token));
}

bool parseUnsignedValue(const std::string& raw, unsigned long long* value, std::string& error) {
    const std::string trimmed = trim(raw);
    if (trimmed.empty()) {
        error = "value is empty";
        return false;
    }

    std::size_t consumed = 0;
    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(trimmed, &consumed, 10);
    } catch (...) {
        error = "expected a non-negative integer";
        return false;
    }

    if (consumed != trimmed.size()) {
        error = "unexpected characters in integer value";
        return false;
    }

    *value = parsed;
    return true;
}

bool parseByteSize(const std::string& raw, uint64_t* value, std::string& error) {
    const std::string trimmed = trim(raw);
    if (trimmed.empty()) {
        error = "size is empty";
        return false;
    }

    std::size_t split = 0;
    bool sawDigit = false;
    while (split < trimmed.size()) {
        const char ch = trimmed[split];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '.') {
            sawDigit = true;
            ++split;
            continue;
        }
        break;
    }

    if (!sawDigit) {
        error = "size must start with a number";
        return false;
    }

    double numericValue = 0.0;
    try {
        numericValue = std::stod(trimmed.substr(0, split));
    } catch (...) {
        error = "invalid size number";
        return false;
    }

    if (numericValue < 0.0) {
        error = "size cannot be negative";
        return false;
    }

    const std::string suffix = toLower(trim(trimmed.substr(split)));
    uint64_t multiplier = 1ULL;
    if (suffix.empty() || suffix == "b") {
        multiplier = 1ULL;
    } else if (suffix == "k" || suffix == "kb" || suffix == "kib") {
        multiplier = 1024ULL;
    } else if (suffix == "m" || suffix == "mb" || suffix == "mib") {
        multiplier = 1024ULL * 1024ULL;
    } else if (suffix == "g" || suffix == "gb" || suffix == "gib") {
        multiplier = 1024ULL * 1024ULL * 1024ULL;
    } else if (suffix == "t" || suffix == "tb" || suffix == "tib") {
        multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    } else {
        error = "unsupported size suffix";
        return false;
    }

    const long double bytes = static_cast<long double>(numericValue) * static_cast<long double>(multiplier);
    if (bytes > static_cast<long double>(std::numeric_limits<uint64_t>::max())) {
        error = "size exceeds supported range";
        return false;
    }

    *value = static_cast<uint64_t>(std::llround(bytes));
    return true;
}

bool parseSortMode(const std::string& raw, SortMode* mode) {
    const std::string normalized = toLower(trim(raw));
    if (normalized == "native" || normalized == "default") {
        *mode = SortMode::Native;
        return true;
    }
    if (normalized == "size" || normalized == "size-desc" || normalized == "largest") {
        *mode = SortMode::SizeDesc;
        return true;
    }
    if (normalized == "size-asc" || normalized == "smallest") {
        *mode = SortMode::SizeAsc;
        return true;
    }
    if (normalized == "time" || normalized == "time-newest" || normalized == "newest") {
        *mode = SortMode::TimeNewest;
        return true;
    }
    if (normalized == "time-oldest" || normalized == "oldest") {
        *mode = SortMode::TimeOldest;
        return true;
    }
    if (normalized == "path" || normalized == "path-asc") {
        *mode = SortMode::PathAsc;
        return true;
    }
    if (normalized == "category") {
        *mode = SortMode::Category;
        return true;
    }
    return false;
}

void appendOptionTable(std::ostringstream& out,
                       std::string_view title,
                       const std::vector<std::pair<std::string, std::string>>& rows) {
    const std::vector<std::size_t> widths {22, 65};
    out << ReportFormatter::section(title);
    out << ReportFormatter::tableBorder(widths) << '\n';
    out << ReportFormatter::tableRow({"Name", "Description"}, widths) << '\n';
    out << ReportFormatter::tableBorder(widths) << '\n';
    for (const auto& row : rows) {
        out << ReportFormatter::tableRow({row.first, row.second}, widths) << '\n';
    }
    out << ReportFormatter::tableBorder(widths) << "\n\n";
}

}  // namespace

bool CliOptions::hasSelection() const {
    return selectAll || !categoryTokens.empty() || !presetTokens.empty();
}

bool CliOptions::hasAdvancedFilters() const {
    return minSizeBytes.has_value() || maxSizeBytes.has_value() || modifiedWithinDays.has_value() ||
           olderThanDays.has_value() || limit.has_value() || sortMode != SortMode::Native;
}

CliOptions CliParser::parse(int argc, char** argv) const {
    CliOptions options;

    if (argc <= 1) {
        options.showHelp = true;
        return options;
    }

    const std::string command = toLower(argv[1]);
    if (command == "scan") {
        options.command = CommandType::Scan;
    } else if (command == "preview") {
        options.command = CommandType::Preview;
    } else if (command == "clean") {
        options.command = CommandType::Clean;
    } else if (command == "report") {
        options.command = CommandType::Report;
    } else if (command == "analyze") {
        options.command = CommandType::Analyze;
    } else if (command == "categories" || command == "list-categories") {
        options.command = CommandType::Categories;
    } else if (command == "help" || command == "--help" || command == "-h") {
        options.command = CommandType::Help;
        options.showHelp = true;
        return options;
    } else {
        options.errors.push_back("Unknown command: " + std::string(argv[1]));
        options.showHelp = true;
        return options;
    }

    for (int index = 2; index < argc; ++index) {
        const std::string argument = toLower(argv[index]);

        if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
        } else if (argument == "--all") {
            options.selectAll = true;
        } else if (argument == "--temp") {
            addCategoryToken(options, "temp");
        } else if (argument == "--logs") {
            addCategoryToken(options, "logs");
        } else if (argument == "--browser") {
            addCategoryToken(options, "browser-cache");
        } else if (argument == "--thumbnails") {
            addCategoryToken(options, "thumbnail-cache");
        } else if (argument == "--shader-cache") {
            addCategoryToken(options, "shader-cache");
        } else if (argument == "--crash-dumps") {
            addCategoryToken(options, "crash-dumps");
        } else if (argument == "--recent") {
            addCategoryToken(options, "recent-artifacts");
        } else if (argument == "--recycle-bin") {
            addCategoryToken(options, "recycle-bin");
        } else if (argument == "--dry-run") {
            options.dryRun = true;
        } else if (argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--yes") {
            options.assumeYes = true;
        } else if (argument == "--export") {
            if (index + 1 >= argc) {
                options.errors.push_back("--export requires a file path.");
                break;
            }
            options.exportPath = std::string(argv[++index]);
        } else if (argument == "--json") {
            if (index + 1 >= argc) {
                options.errors.push_back("--json requires a file path.");
                break;
            }
            options.jsonPath = std::string(argv[++index]);
        } else if (argument == "--category") {
            if (index + 1 >= argc) {
                options.errors.push_back("--category requires a value.");
                break;
            }
            addCategoryToken(options, argv[++index]);
        } else if (argument == "--preset") {
            if (index + 1 >= argc) {
                options.errors.push_back("--preset requires a value.");
                break;
            }
            addPresetToken(options, argv[++index]);
        } else if (argument == "--min-size") {
            if (index + 1 >= argc) {
                options.errors.push_back("--min-size requires a value.");
                break;
            }

            std::string error;
            uint64_t parsed = 0;
            if (!parseByteSize(argv[++index], &parsed, error)) {
                options.errors.push_back("--min-size: " + error);
                continue;
            }
            options.minSizeBytes = parsed;
        } else if (argument == "--max-size") {
            if (index + 1 >= argc) {
                options.errors.push_back("--max-size requires a value.");
                break;
            }

            std::string error;
            uint64_t parsed = 0;
            if (!parseByteSize(argv[++index], &parsed, error)) {
                options.errors.push_back("--max-size: " + error);
                continue;
            }
            options.maxSizeBytes = parsed;
        } else if (argument == "--modified-within-days") {
            if (index + 1 >= argc) {
                options.errors.push_back("--modified-within-days requires a value.");
                break;
            }

            std::string error;
            unsigned long long parsed = 0;
            if (!parseUnsignedValue(argv[++index], &parsed, error) ||
                parsed > static_cast<unsigned long long>(std::numeric_limits<uint32_t>::max())) {
                options.errors.push_back("--modified-within-days: expected a 32-bit non-negative integer.");
                continue;
            }
            options.modifiedWithinDays = static_cast<uint32_t>(parsed);
        } else if (argument == "--older-than-days") {
            if (index + 1 >= argc) {
                options.errors.push_back("--older-than-days requires a value.");
                break;
            }

            std::string error;
            unsigned long long parsed = 0;
            if (!parseUnsignedValue(argv[++index], &parsed, error) ||
                parsed > static_cast<unsigned long long>(std::numeric_limits<uint32_t>::max())) {
                options.errors.push_back("--older-than-days: expected a 32-bit non-negative integer.");
                continue;
            }
            options.olderThanDays = static_cast<uint32_t>(parsed);
        } else if (argument == "--limit") {
            if (index + 1 >= argc) {
                options.errors.push_back("--limit requires a value.");
                break;
            }

            std::string error;
            unsigned long long parsed = 0;
            if (!parseUnsignedValue(argv[++index], &parsed, error) || parsed == 0ULL) {
                options.errors.push_back("--limit: expected a positive integer.");
                continue;
            }
            options.limit = static_cast<std::size_t>(parsed);
        } else if (argument == "--sort") {
            if (index + 1 >= argc) {
                options.errors.push_back("--sort requires a value.");
                break;
            }

            SortMode mode = SortMode::Native;
            if (!parseSortMode(argv[++index], &mode)) {
                options.errors.push_back("--sort supports: native, size, size-asc, newest, oldest, path, category.");
                continue;
            }
            options.sortMode = mode;
        } else {
            options.errors.push_back("Unknown argument: " + std::string(argv[index]));
        }
    }

    if (options.minSizeBytes.has_value() && options.maxSizeBytes.has_value() &&
        *options.minSizeBytes > *options.maxSizeBytes) {
        options.errors.push_back("--min-size cannot be larger than --max-size.");
    }

    if (options.modifiedWithinDays.has_value() && options.olderThanDays.has_value() &&
        *options.olderThanDays > *options.modifiedWithinDays) {
        options.errors.push_back("--older-than-days cannot exceed --modified-within-days for the same query.");
    }

    return options;
}

std::string CliParser::helpText() const {
    std::ostringstream out;
    out << ReportFormatter::logo() << '\n';
    out << ReportFormatter::banner("FCASHER CLI REFERENCE",
                                   "Preview-first cache inspection, analysis, and controlled cleanup for Windows.")
        << "\n\n";
    out << ReportFormatter::keyValue("Usage", "fcasher <command> [options]");
    out << ReportFormatter::keyValue("Model", "Inspect first, filter intentionally, then delete explicitly.");
    out << ReportFormatter::keyValue("Safety", "Protected roots, user-content paths, and explicit-only zones stay constrained.");
    out << ReportFormatter::keyValue("Power", "Supports presets, sorting, limits, size filters, and category inventory.")
        << '\n';

    appendOptionTable(out,
                      "Commands",
                      {
                          {"scan", "Inspect supported locations and print a categorized report."},
                          {"preview", "Scan and emphasize files selected for cleanup."},
                          {"clean", "Scan, filter, preview, confirm, then delete eligible files."},
                          {"report", "Scan and optionally export TXT and JSON reports."},
                          {"analyze", "Scan and produce a ranked analysis view for power users."},
                          {"categories", "List built-in categories, roots, presets, and safety notes."}
                      });

    appendOptionTable(out,
                      "Selectors",
                      {
                          {"--all", "Select every non-destructive built-in category."},
                          {"--temp", "Select user temp, local app temp, and Windows temp."},
                          {"--logs", "Select log and diagnostic artifacts."},
                          {"--browser", "Select browser cache directories."},
                          {"--thumbnails", "Select thumbnail and icon cache files."},
                          {"--shader-cache", "Select graphics shader caches."},
                          {"--crash-dumps", "Select crash dump locations."},
                          {"--recent", "Select recent temporary artifacts."},
                          {"--recycle-bin", "Select recycle bin roots. Explicit opt-in only."},
                          {"--category NAME", "Select a category by canonical key."},
                          {"--preset NAME", "Apply a named built-in preset such as quick-sweep or diagnostics."}
                      });

    appendOptionTable(out,
                      "Filters",
                      {
                          {"--min-size SIZE", "Keep only files at or above SIZE. Supports KB/MB/GB/TB."},
                          {"--max-size SIZE", "Keep only files at or below SIZE."},
                          {"--modified-within-days N", "Keep only files modified within the last N days."},
                          {"--older-than-days N", "Keep only files older than N days."},
                          {"--sort MODE", "Sort by native, size, size-asc, newest, oldest, path, or category."},
                          {"--limit N", "Limit the effective result set after filtering and sorting."}
                      });

    appendOptionTable(out,
                      "Flags",
                      {
                          {"--dry-run", "Simulate cleanup without deleting files."},
                          {"--verbose", "Emit additional workflow messages."},
                          {"--yes", "Skip interactive confirmation for clean."},
                          {"--export PATH", "Export the primary text report to PATH."},
                          {"--json PATH", "Export scan results to PATH as JSON."},
                          {"--help", "Show this help text."}
                      });

    out << ReportFormatter::section("Examples");
    out << "  fcasher scan --all\n";
    out << "  fcasher preview --preset quick-sweep --sort size --limit 25\n";
    out << "  fcasher analyze --all --min-size 8MB --sort size --limit 40\n";
    out << "  fcasher analyze --preset diagnostics --older-than-days 14 --json reports\\analysis.json\n";
    out << "  fcasher clean --category temp --max-size 128MB --dry-run\n";
    out << "  fcasher categories\n";
    return out.str();
}

}  // namespace fcasher::app
