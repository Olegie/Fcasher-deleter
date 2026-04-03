#include "services/export_service.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>

#include <windows.h>

#include "app/report_formatter.hpp"
#include "core/file_record.h"

namespace fcasher::services {
namespace {

std::string normalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '/', '\\');
    return value;
}

std::string jsonEscape(std::string_view value) {
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

bool ensureDirectoryChain(const std::string& path) {
    const std::string normalized = normalizePath(path);
    const std::size_t lastSlash = normalized.find_last_of('\\');
    if (lastSlash == std::string::npos) {
        return true;
    }

    const std::string directory = normalized.substr(0, lastSlash);
    if (directory.empty()) {
        return true;
    }

    std::string current;
    std::size_t position = 0;

    if (directory.size() >= 2 && directory[1] == ':') {
        current = directory.substr(0, 2);
        position = 2;
        if (position < directory.size() && directory[position] == '\\') {
            current.push_back('\\');
            ++position;
        }
    }

    while (position < directory.size()) {
        const std::size_t next = directory.find('\\', position);
        const std::string part = directory.substr(position, next - position);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '\\') {
                current.push_back('\\');
            }
            current += part;
            if (CreateDirectoryA(current.c_str(), nullptr) == 0) {
                const DWORD errorCode = GetLastError();
                if (errorCode != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
        if (next == std::string::npos) {
            break;
        }
        position = next + 1;
    }

    return true;
}

}  // namespace

bool ExportService::writeText(const std::string& path, std::string_view content, std::string& error) const {
    if (!ensureDirectoryChain(path)) {
        error = "Unable to create export directory chain for: " + path;
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Unable to open export file: " + path;
        return false;
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output.good()) {
        error = "Unable to write export file: " + path;
        return false;
    }

    return true;
}

bool ExportService::writeScanJson(const std::string& path,
                                  const fc_scan_result& result,
                                  const std::vector<const fcasher::app::CategoryDefinition*>& categories,
                                  std::string& error) const {
    std::ostringstream out;
    out << "{\n";
    out << "  \"tool\": \"Fcasher\",\n";
    out << "  \"generated_at_utc\": \""
        << jsonEscape(fcasher::app::ReportFormatter::utcTimestamp(static_cast<uint64_t>(std::time(nullptr))))
        << "\",\n";
    out << "  \"selected_categories\": [\n";
    for (std::size_t index = 0; index < categories.size(); ++index) {
        const auto* category = categories[index];
        out << "    {\"key\": \"" << jsonEscape(category->key)
            << "\", \"display_name\": \"" << jsonEscape(category->displayName) << "\"}";
        out << (index + 1 == categories.size() ? '\n' : ',');
    }
    out << "  ],\n";
    out << "  \"summary\": {\n";
    out << "    \"file_count\": " << result.file_count << ",\n";
    out << "    \"total_size_bytes\": " << result.total_size_bytes << ",\n";
    out << "    \"skipped_count\": " << result.skipped_count << "\n";
    out << "  },\n";

    out << "  \"scanned_directories\": [\n";
    for (std::size_t index = 0; index < result.scanned_directory_count; ++index) {
        out << "    \"" << jsonEscape(result.scanned_directories[index]) << "\"";
        out << (index + 1 == result.scanned_directory_count ? '\n' : ',');
    }
    out << "  ],\n";

    out << "  \"category_summary\": [\n";
    for (std::size_t index = 0; index < categories.size(); ++index) {
        const auto* category = categories[index];
        out << "    {\"key\": \"" << jsonEscape(category->key)
            << "\", \"file_count\": " << fc_scan_result_count_for_category(&result, category->id)
            << ", \"size_bytes\": " << fc_scan_result_size_for_category(&result, category->id)
            << "}";
        out << (index + 1 == categories.size() ? '\n' : ',');
    }
    out << "  ],\n";

    out << "  \"files\": [\n";
    for (std::size_t index = 0; index < result.file_count; ++index) {
        const auto& record = result.files[index];
        out << "    {\"path\": \"" << jsonEscape(record.path)
            << "\", \"category\": \"" << jsonEscape(fc_category_name(record.category))
            << "\", \"size_bytes\": " << record.size_bytes
            << ", \"last_write_utc\": \"" << jsonEscape(fcasher::app::ReportFormatter::utcTimestamp(record.last_write_unix))
            << "\", \"reason\": \"" << jsonEscape(record.reason) << "\"}";
        out << (index + 1 == result.file_count ? '\n' : ',');
    }
    out << "  ],\n";

    out << "  \"skipped\": [\n";
    for (std::size_t index = 0; index < result.skipped_count; ++index) {
        out << "    {\"path\": \"" << jsonEscape(result.skipped[index].path)
            << "\", \"reason\": \"" << jsonEscape(result.skipped[index].reason) << "\"}";
        out << (index + 1 == result.skipped_count ? '\n' : ',');
    }
    out << "  ]\n";
    out << "}\n";

    return writeText(path, out.str(), error);
}

}  // namespace fcasher::services
