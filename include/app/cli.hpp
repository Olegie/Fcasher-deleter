#ifndef FCASHER_APP_CLI_HPP
#define FCASHER_APP_CLI_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fcasher::app {

enum class CommandType {
    Help,
    Scan,
    Preview,
    Clean,
    Report,
    Analyze,
    Categories
};

enum class SortMode {
    Native,
    SizeDesc,
    SizeAsc,
    TimeNewest,
    TimeOldest,
    PathAsc,
    Category
};

struct CliOptions {
    CommandType command{CommandType::Help};
    bool showHelp{false};
    bool selectAll{false};
    bool dryRun{false};
    bool verbose{false};
    bool assumeYes{false};
    std::vector<std::string> categoryTokens;
    std::vector<std::string> presetTokens;
    std::optional<std::string> exportPath;
    std::optional<std::string> jsonPath;
    std::optional<uint64_t> minSizeBytes;
    std::optional<uint64_t> maxSizeBytes;
    std::optional<uint32_t> modifiedWithinDays;
    std::optional<uint32_t> olderThanDays;
    std::optional<std::size_t> limit;
    SortMode sortMode{SortMode::Native};
    std::vector<std::string> errors;

    bool hasSelection() const;
    bool hasAdvancedFilters() const;
};

class CliParser {
public:
    CliOptions parse(int argc, char** argv) const;
    std::string helpText() const;
};

}  // namespace fcasher::app

#endif
