#include "app/report_formatter.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fcasher::app {
namespace {

constexpr std::size_t kBannerWidth = 94;
constexpr std::size_t kMinimumWrappedValueWidth = 24;

bool isWrapBreakChar(char ch) {
    return ch == ' ' || ch == '\\' || ch == '/' || ch == ':' || ch == ';' || ch == ',' || ch == '-' || ch == '_';
}

std::vector<std::string> wrapText(std::string_view value, std::size_t width) {
    std::vector<std::string> lines;
    std::size_t cursor = 0;

    if (value.empty()) {
        lines.push_back({});
        return lines;
    }

    if (width == 0) {
        lines.push_back(std::string(value));
        return lines;
    }

    while (cursor < value.size()) {
        const std::size_t remaining = value.size() - cursor;
        if (remaining <= width) {
            lines.emplace_back(value.substr(cursor));
            break;
        }

        std::size_t split = cursor + width;
        for (std::size_t probe = split; probe > cursor; --probe) {
            if (isWrapBreakChar(value[probe - 1])) {
                split = probe;
                break;
            }
        }

        if (split == cursor) {
            split = cursor + width;
        }

        lines.emplace_back(value.substr(cursor, split - cursor));
        cursor = split;
        while (cursor < value.size() && value[cursor] == ' ') {
            ++cursor;
        }
    }

    return lines;
}

std::string fitCell(std::string_view value, std::size_t width) {
    if (value.size() <= width) {
        return std::string(value);
    }
    if (width <= 3) {
        return std::string(width, '.');
    }
    return std::string(value.substr(0, width - 3)) + "...";
}

std::string boxLine(std::string_view text) {
    constexpr std::size_t kInnerWidth = kBannerWidth - 4;
    return "| " + ReportFormatter::padRight(text, kInnerWidth) + " |";
}

}  // namespace

std::string ReportFormatter::humanSize(uint64_t bytes) {
    static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    std::size_t unitIndex = 0;

    while (value >= 1024.0 && unitIndex + 1 < (sizeof(kUnits) / sizeof(kUnits[0]))) {
        value /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream out;
    if (unitIndex == 0) {
        out << static_cast<uint64_t>(value) << ' ' << kUnits[unitIndex];
    } else {
        out << std::fixed << std::setprecision(value < 10.0 ? 2 : 1) << value << ' ' << kUnits[unitIndex];
    }
    return out.str();
}

std::string ReportFormatter::logo() {
    return
        "   ______ ______    ___   _____ __  __ ______ _____\n"
        "  / ____// __  /   /   | / ___// / / / ____// __ \\\n"
        " / /_   / /       / /| | \\__ \\/ /_/ / __/  / /_/ /\n"
        "/ __/  / /       / ___ |___/ / __  / /___ / _, _/\n"
        "/_/   /____/    /_/  |_/____/_/ /_/_____//_/ |_|\n"
        "                 F C A S H E R\n";
}

std::string ReportFormatter::banner(std::string_view title, std::string_view subtitle) {
    std::ostringstream out;
    out << '+' << line('-', kBannerWidth - 2) << "+\n";
    out << boxLine(title) << "\n";
    if (!subtitle.empty()) {
        out << boxLine(subtitle) << "\n";
    }
    out << '+' << line('-', kBannerWidth - 2) << '+';
    return out.str();
}

std::string ReportFormatter::section(std::string_view title) {
    std::ostringstream out;
    out << '[' << ' ' << title << ' ' << ']' << "\n";
    out << line('-', kBannerWidth) << '\n';
    return out.str();
}

std::string ReportFormatter::keyValue(std::string_view key, std::string_view value, std::size_t keyWidth) {
    const std::size_t valueWidth =
        (kBannerWidth > keyWidth + 3U) ? std::max(kMinimumWrappedValueWidth, kBannerWidth - keyWidth - 3U)
                                       : kMinimumWrappedValueWidth;
    const auto lines = wrapText(value, valueWidth);
    const std::string continuationPrefix(keyWidth + 3U, ' ');

    std::ostringstream out;
    out << padRight(key, keyWidth) << " : " << lines.front() << '\n';
    for (std::size_t index = 1; index < lines.size(); ++index) {
        out << continuationPrefix << lines[index] << '\n';
    }
    return out.str();
}

std::string ReportFormatter::line(char ch, std::size_t count) {
    return std::string(count, ch);
}

std::string ReportFormatter::padRight(std::string_view value, std::size_t width) {
    if (value.size() >= width) {
        return std::string(value);
    }
    return std::string(value) + std::string(width - value.size(), ' ');
}

std::string ReportFormatter::tableBorder(const std::vector<std::size_t>& widths) {
    std::ostringstream out;
    out << '+';
    for (const std::size_t width : widths) {
        out << line('-', width + 2) << '+';
    }
    return out.str();
}

std::string ReportFormatter::tableRow(const std::vector<std::string>& columns,
                                      const std::vector<std::size_t>& widths) {
    std::ostringstream out;
    out << '|';

    const std::size_t count = std::min(columns.size(), widths.size());
    for (std::size_t index = 0; index < count; ++index) {
        out << ' ' << padRight(fitCell(columns[index], widths[index]), widths[index]) << ' ' << '|';
    }

    for (std::size_t index = count; index < widths.size(); ++index) {
        out << ' ' << std::string(widths[index], ' ') << ' ' << '|';
    }

    return out.str();
}

std::string ReportFormatter::utcTimestamp(uint64_t unixSeconds) {
    if (unixSeconds == 0) {
        return "n/a";
    }

    std::time_t raw = static_cast<std::time_t>(unixSeconds);
    std::tm tmValue {};

#if defined(_MSC_VER)
    gmtime_s(&tmValue, &raw);
#elif defined(__MINGW32__) || defined(__MINGW64__)
    if (const std::tm* utcValue = std::gmtime(&raw)) {
        tmValue = *utcValue;
    } else {
        return "n/a";
    }
#else
    gmtime_r(&raw, &tmValue);
#endif

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%SZ", &tmValue);
    return buffer;
}

}  // namespace fcasher::app
