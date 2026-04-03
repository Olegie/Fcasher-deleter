#ifndef FCASHER_APP_CLI_HPP
#define FCASHER_APP_CLI_HPP

#include <optional>
#include <string>
#include <vector>

namespace fcasher::app {

enum class CommandType {
    Help,
    Scan,
    Preview,
    Clean,
    Report
};

struct CliOptions {
    CommandType command{CommandType::Help};
    bool showHelp{false};
    bool selectAll{false};
    bool dryRun{false};
    bool verbose{false};
    bool assumeYes{false};
    std::vector<std::string> categoryTokens;
    std::optional<std::string> exportPath;
    std::optional<std::string> jsonPath;
    std::vector<std::string> errors;

    bool hasSelection() const;
};

class CliParser {
public:
    CliOptions parse(int argc, char** argv) const;
    std::string helpText() const;
};

}  // namespace fcasher::app

#endif
