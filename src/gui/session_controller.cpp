#include "gui/session_controller.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <windows.h>

#include "app/cli.hpp"
#include "app/command_dispatcher.hpp"
#include "app/report_formatter.hpp"
#include "services/export_service.hpp"

namespace fcasher::gui {
namespace {

struct JsonValue {
    enum class Type {
        Null,
        Object,
        Array,
        String,
        Number,
        Bool
    };

    Type type{Type::Null};
    std::vector<std::pair<std::string, JsonValue>> objectValue;
    std::vector<JsonValue> arrayValue;
    std::string stringValue;
    uint64_t numberValue{0};
    bool boolValue{false};

    const JsonValue* member(std::string_view key) const {
        if (type != Type::Object) {
            return nullptr;
        }

        for (const auto& entry : objectValue) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    bool parse(JsonValue* value) {
        skipWhitespace();
        if (!parseValue(value)) {
            return false;
        }
        skipWhitespace();
        return position_ == text_.size();
    }

private:
    void skipWhitespace() {
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                break;
            }
            ++position_;
        }
    }

    bool consume(char expected) {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    bool parseValue(JsonValue* value) {
        skipWhitespace();
        if (position_ >= text_.size()) {
            return false;
        }

        const char ch = text_[position_];
        if (ch == '{') {
            return parseObject(value);
        }
        if (ch == '[') {
            return parseArray(value);
        }
        if (ch == '"') {
            value->type = JsonValue::Type::String;
            return parseString(&value->stringValue);
        }
        if ((ch >= '0' && ch <= '9') || ch == '-') {
            value->type = JsonValue::Type::Number;
            return parseNumber(&value->numberValue);
        }
        if (matchLiteral("true")) {
            value->type = JsonValue::Type::Bool;
            value->boolValue = true;
            return true;
        }
        if (matchLiteral("false")) {
            value->type = JsonValue::Type::Bool;
            value->boolValue = false;
            return true;
        }
        if (matchLiteral("null")) {
            value->type = JsonValue::Type::Null;
            return true;
        }

        return false;
    }

    bool parseObject(JsonValue* value) {
        if (!consume('{')) {
            return false;
        }

        value->type = JsonValue::Type::Object;
        value->objectValue.clear();
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return true;
        }

        while (position_ < text_.size()) {
            std::string key;
            JsonValue memberValue;
            if (!parseString(&key)) {
                return false;
            }
            if (!consume(':') || !parseValue(&memberValue)) {
                return false;
            }
            value->objectValue.push_back({std::move(key), std::move(memberValue)});
            skipWhitespace();
            if (position_ < text_.size() && text_[position_] == '}') {
                ++position_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }

        return false;
    }

    bool parseArray(JsonValue* value) {
        if (!consume('[')) {
            return false;
        }

        value->type = JsonValue::Type::Array;
        value->arrayValue.clear();
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == ']') {
            ++position_;
            return true;
        }

        while (position_ < text_.size()) {
            JsonValue item;
            if (!parseValue(&item)) {
                return false;
            }
            value->arrayValue.push_back(std::move(item));
            skipWhitespace();
            if (position_ < text_.size() && text_[position_] == ']') {
                ++position_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }

        return false;
    }

    bool parseString(std::string* out) {
        if (!consume('"')) {
            return false;
        }

        out->clear();
        while (position_ < text_.size()) {
            const char ch = text_[position_++];
            if (ch == '"') {
                return true;
            }
            if (ch != '\\') {
                out->push_back(ch);
                continue;
            }

            if (position_ >= text_.size()) {
                return false;
            }

            const char escape = text_[position_++];
            switch (escape) {
            case '"':
            case '\\':
            case '/':
                out->push_back(escape);
                break;
            case 'b':
                out->push_back('\b');
                break;
            case 'f':
                out->push_back('\f');
                break;
            case 'n':
                out->push_back('\n');
                break;
            case 'r':
                out->push_back('\r');
                break;
            case 't':
                out->push_back('\t');
                break;
            case 'u':
                if (position_ + 4U > text_.size()) {
                    return false;
                }
                position_ += 4U;
                out->push_back('?');
                break;
            default:
                return false;
            }
        }

        return false;
    }

    bool parseNumber(uint64_t* out) {
        std::size_t start = position_;
        if (text_[position_] == '-') {
            ++position_;
        }
        while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
            ++position_;
        }

        try {
            *out = static_cast<uint64_t>(std::stoull(std::string(text_.substr(start, position_ - start))));
        } catch (...) {
            return false;
        }

        return true;
    }

    bool matchLiteral(std::string_view literal) {
        skipWhitespace();
        if (text_.substr(position_, literal.size()) != literal) {
            return false;
        }
        position_ += literal.size();
        return true;
    }

    std::string_view text_;
    std::size_t position_{0};
};

std::string commandLabel(fcasher::app::CommandType command) {
    switch (command) {
    case fcasher::app::CommandType::Scan:
        return "Scan";
    case fcasher::app::CommandType::Preview:
        return "Preview";
    case fcasher::app::CommandType::Clean:
        return "Clean";
    case fcasher::app::CommandType::Report:
        return "Report";
    case fcasher::app::CommandType::Analyze:
        return "Analyze";
    case fcasher::app::CommandType::Categories:
        return "Categories";
    case fcasher::app::CommandType::Help:
    default:
        return "Help";
    }
}

std::string sortLabel(fcasher::app::SortMode mode) {
    switch (mode) {
    case fcasher::app::SortMode::SizeDesc:
        return "Largest First";
    case fcasher::app::SortMode::SizeAsc:
        return "Smallest First";
    case fcasher::app::SortMode::TimeNewest:
        return "Newest First";
    case fcasher::app::SortMode::TimeOldest:
        return "Oldest First";
    case fcasher::app::SortMode::PathAsc:
        return "Path";
    case fcasher::app::SortMode::Category:
        return "Category";
    case fcasher::app::SortMode::Native:
    default:
        return "Native";
    }
}

std::string titleFromKey(std::string value) {
    bool capitalize = true;
    for (char& ch : value) {
        if (ch == '-' || ch == '_') {
            ch = ' ';
            capitalize = true;
            continue;
        }

        if (capitalize && ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        } else if (!capitalize && ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
        capitalize = (ch == ' ');
    }
    return value;
}

std::string joinStrings(const std::vector<std::string>& values) {
    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            out << ", ";
        }
        out << values[index];
    }
    return out.str();
}

std::string readWholeFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    std::ostringstream out;
    out << input.rdbuf();
    return out.str();
}

std::string tempJsonPath() {
    char tempDirectory[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(MAX_PATH, tempDirectory) == 0) {
        return {};
    }
    if (GetTempFileNameA(tempDirectory, "fcr", 0, tempFile) == 0) {
        return {};
    }

    std::string path = tempFile;
    const std::size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        path.erase(dot);
    }
    path += ".json";
    DeleteFileA(path.c_str());
    return path;
}

void addSummary(StructuredResult* structured, std::string label, std::string value) {
    if (value.empty()) {
        return;
    }
    structured->summary.push_back({std::move(label), std::move(value)});
}

bool populateStructuredResult(const JsonValue& root,
                              const fcasher::app::CliOptions& options,
                              StructuredResult* structured) {
    const JsonValue* summary = root.member("summary");
    const JsonValue* selectedCategories = root.member("selected_categories");
    const JsonValue* scannedDirectories = root.member("scanned_directories");
    const JsonValue* categorySummary = root.member("category_summary");
    const JsonValue* files = root.member("files");
    const JsonValue* skipped = root.member("skipped");

    if (summary == nullptr || summary->type != JsonValue::Type::Object || files == nullptr ||
        files->type != JsonValue::Type::Array || skipped == nullptr || skipped->type != JsonValue::Type::Array) {
        return false;
    }

    structured->available = true;
    addSummary(structured, "Mode", commandLabel(options.command));

    std::vector<std::string> selectedNames;
    if (selectedCategories != nullptr && selectedCategories->type == JsonValue::Type::Array) {
        for (const auto& entry : selectedCategories->arrayValue) {
            if (const JsonValue* name = entry.member("display_name")) {
                if (name->type == JsonValue::Type::String) {
                    selectedNames.push_back(name->stringValue);
                }
            }
        }
    }
    addSummary(structured, "Selection", selectedNames.empty() ? "Built-in defaults" : joinStrings(selectedNames));

    if (options.selectAll) {
        addSummary(structured, "Profile", "All safe categories");
    }
    if (!options.presetTokens.empty()) {
        std::vector<std::string> presets;
        presets.reserve(options.presetTokens.size());
        for (const auto& preset : options.presetTokens) {
            presets.push_back(titleFromKey(preset));
        }
        addSummary(structured, "Preset", joinStrings(presets));
    }

    if (const JsonValue* value = summary->member("file_count")) {
        if (value->type == JsonValue::Type::Number) {
            addSummary(structured, "Files", std::to_string(value->numberValue));
        }
    }
    if (const JsonValue* value = summary->member("total_size_bytes")) {
        if (value->type == JsonValue::Type::Number) {
            addSummary(structured, "Size", fcasher::app::ReportFormatter::humanSize(value->numberValue));
        }
    }
    if (const JsonValue* value = summary->member("skipped_count")) {
        if (value->type == JsonValue::Type::Number) {
            addSummary(structured, "Skipped", std::to_string(value->numberValue));
        }
    }
    if (scannedDirectories != nullptr && scannedDirectories->type == JsonValue::Type::Array) {
        addSummary(structured, "Scanned folders", std::to_string(scannedDirectories->arrayValue.size()));
    }
    addSummary(structured, "Sort", sortLabel(options.sortMode));

    std::vector<std::string> filters;
    if (options.minSizeBytes.has_value()) {
        filters.push_back("Min " + fcasher::app::ReportFormatter::humanSize(*options.minSizeBytes));
    }
    if (options.maxSizeBytes.has_value()) {
        filters.push_back("Max " + fcasher::app::ReportFormatter::humanSize(*options.maxSizeBytes));
    }
    if (options.modifiedWithinDays.has_value()) {
        filters.push_back("Within " + std::to_string(*options.modifiedWithinDays) + "d");
    }
    if (options.olderThanDays.has_value()) {
        filters.push_back("Older than " + std::to_string(*options.olderThanDays) + "d");
    }
    if (options.limit.has_value()) {
        filters.push_back("Limit " + std::to_string(*options.limit));
    }
    if (options.dryRun) {
        filters.push_back("Dry run");
    }
    addSummary(structured, "Filters", filters.empty() ? "None" : joinStrings(filters));

    if (categorySummary != nullptr && categorySummary->type == JsonValue::Type::Array) {
        for (const auto& entry : categorySummary->arrayValue) {
            const JsonValue* key = entry.member("key");
            const JsonValue* fileCount = entry.member("file_count");
            const JsonValue* sizeBytes = entry.member("size_bytes");
            if (key == nullptr || fileCount == nullptr || sizeBytes == nullptr ||
                key->type != JsonValue::Type::String || fileCount->type != JsonValue::Type::Number ||
                sizeBytes->type != JsonValue::Type::Number) {
                continue;
            }

            addSummary(structured,
                       titleFromKey(key->stringValue),
                       std::to_string(fileCount->numberValue) + " files, " +
                           fcasher::app::ReportFormatter::humanSize(sizeBytes->numberValue));
        }
    }

    for (const auto& entry : files->arrayValue) {
        const JsonValue* path = entry.member("path");
        const JsonValue* category = entry.member("category");
        const JsonValue* sizeBytes = entry.member("size_bytes");
        const JsonValue* lastWrite = entry.member("last_write_utc");
        const JsonValue* reason = entry.member("reason");
        if (path == nullptr || category == nullptr || sizeBytes == nullptr || lastWrite == nullptr || reason == nullptr ||
            path->type != JsonValue::Type::String || category->type != JsonValue::Type::String ||
            sizeBytes->type != JsonValue::Type::Number || lastWrite->type != JsonValue::Type::String ||
            reason->type != JsonValue::Type::String) {
            continue;
        }

        structured->files.push_back({
            path->stringValue,
            titleFromKey(category->stringValue),
            fcasher::app::ReportFormatter::humanSize(sizeBytes->numberValue),
            lastWrite->stringValue,
            reason->stringValue
        });
    }

    for (const auto& entry : skipped->arrayValue) {
        const JsonValue* path = entry.member("path");
        const JsonValue* reason = entry.member("reason");
        if (path == nullptr || reason == nullptr || path->type != JsonValue::Type::String ||
            reason->type != JsonValue::Type::String) {
            continue;
        }

        structured->skipped.push_back({path->stringValue, reason->stringValue});
    }

    return true;
}

}  // namespace

ExecutionResult SessionController::execute(const std::vector<std::string>& arguments,
                                           bool assumeYesForClean,
                                           const std::optional<std::string>& jsonPath) const {
    fcasher::app::CliParser parser;
    std::vector<std::string> argvStorage;
    argvStorage.reserve(arguments.size() + 1U);
    argvStorage.push_back("fcasher");
    argvStorage.insert(argvStorage.end(), arguments.begin(), arguments.end());

    std::vector<char*> argv;
    argv.reserve(argvStorage.size());
    for (auto& argument : argvStorage) {
        argv.push_back(argument.data());
    }

    fcasher::app::CliOptions options = parser.parse(static_cast<int>(argv.size()), argv.data());
    if (assumeYesForClean) {
        options.assumeYes = true;
    }

    const bool categoriesMode = options.command == fcasher::app::CommandType::Categories;
    std::string generatedJsonPath;
    if (!categoriesMode) {
        generatedJsonPath = jsonPath.has_value() ? *jsonPath : tempJsonPath();
        if (!generatedJsonPath.empty()) {
            options.jsonPath = generatedJsonPath;
        }
    }

    std::ostringstream outCapture;
    std::ostringstream errCapture;
    std::streambuf* oldOut = std::cout.rdbuf(outCapture.rdbuf());
    std::streambuf* oldErr = std::cerr.rdbuf(errCapture.rdbuf());

    fcasher::app::CommandDispatcher dispatcher;
    const int exitCode = dispatcher.run(options);

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    ExecutionResult result;
    result.exitCode = exitCode;
    result.output = outCapture.str();

    const std::string errorOutput = errCapture.str();
    if (!errorOutput.empty()) {
        if (!result.output.empty() && result.output.back() != '\n') {
            result.output.push_back('\n');
        }
        result.output += errorOutput;
    }

    if (!generatedJsonPath.empty()) {
        JsonValue root;
        const std::string jsonText = readWholeFile(generatedJsonPath);
        if (!jsonText.empty()) {
            JsonParser jsonParser(jsonText);
            if (jsonParser.parse(&root)) {
                populateStructuredResult(root, options, &result.structured);
            }
        }

        if (!jsonPath.has_value()) {
            DeleteFileA(generatedJsonPath.c_str());
        }
    }

    return result;
}

bool SessionController::saveTextReport(const std::string& path, std::string_view content, std::string& error) const {
    fcasher::services::ExportService exportService;
    return exportService.writeText(path, content, error);
}

}  // namespace fcasher::gui
