#ifndef FCASHER_APP_REPORT_FORMATTER_HPP
#define FCASHER_APP_REPORT_FORMATTER_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace fcasher::app {

class ReportFormatter {
public:
    static std::string humanSize(uint64_t bytes);
    static std::string line(char ch, std::size_t count);
    static std::string padRight(std::string_view value, std::size_t width);
    static std::string utcTimestamp(uint64_t unixSeconds);
};

}  // namespace fcasher::app

#endif
