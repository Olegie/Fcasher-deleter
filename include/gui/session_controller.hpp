#ifndef FCASHER_GUI_SESSION_CONTROLLER_HPP
#define FCASHER_GUI_SESSION_CONTROLLER_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fcasher::gui {

struct SummaryItem {
    std::string label;
    std::string value;
};

struct FileItem {
    std::string path;
    std::string category;
    std::string size;
    std::string lastWrite;
    std::string reason;
};

struct SkipItem {
    std::string path;
    std::string reason;
};

struct StructuredResult {
    bool available{false};
    std::vector<SummaryItem> summary;
    std::vector<FileItem> files;
    std::vector<SkipItem> skipped;
};

struct ExecutionResult {
    int exitCode{0};
    std::string output;
    StructuredResult structured;
};

class SessionController {
public:
    ExecutionResult execute(const std::vector<std::string>& arguments,
                            bool assumeYesForClean,
                            const std::optional<std::string>& jsonPath = std::nullopt) const;
    bool saveTextReport(const std::string& path, std::string_view content, std::string& error) const;
};

}  // namespace fcasher::gui

#endif
