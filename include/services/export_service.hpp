#ifndef FCASHER_SERVICES_EXPORT_SERVICE_HPP
#define FCASHER_SERVICES_EXPORT_SERVICE_HPP

#include <string>
#include <string_view>
#include <vector>

#include "app/category_registry.hpp"
#include "core/scan_result.h"

namespace fcasher::services {

class ExportService {
public:
    bool writeText(const std::string& path, std::string_view content, std::string& error) const;
    bool writeScanJson(const std::string& path,
                       const fc_scan_result& result,
                       const std::vector<const fcasher::app::CategoryDefinition*>& categories,
                       std::string& error) const;
};

}  // namespace fcasher::services

#endif
