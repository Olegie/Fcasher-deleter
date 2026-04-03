#ifndef FCASHER_APP_REPORT_FORMATTER_HPP
#define FCASHER_APP_REPORT_FORMATTER_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace fcasher::app {

class ReportFormatter {
public:
    static std::string humanSize(uint64_t bytes);
    static std::string logo();
    static std::string banner(std::string_view title, std::string_view subtitle);
    static std::string section(std::string_view title);
    static std::string keyValue(std::string_view key, std::string_view value, std::size_t keyWidth = 20);
    static std::string line(char ch, std::size_t count);
    static std::string padRight(std::string_view value, std::size_t width);
    static std::string tableBorder(const std::vector<std::size_t>& widths);
    static std::string tableRow(const std::vector<std::string>& columns, const std::vector<std::size_t>& widths);
    static std::string utcTimestamp(uint64_t unixSeconds);
};

}  // namespace fcasher::app

#endif
