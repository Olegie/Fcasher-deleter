#ifndef FCASHER_SERVICES_REPORT_SERVICE_HPP
#define FCASHER_SERVICES_REPORT_SERVICE_HPP

#include <string>
#include <vector>

#include "app/category_registry.hpp"
#include "core/cleanup_engine.h"
#include "core/scan_result.h"

namespace fcasher::services {

struct ReportContext {
    std::string mode;
    std::vector<const fcasher::app::CategoryDefinition*> categories;
    bool dryRun{false};
    bool verbose{false};
};

class ReportService {
public:
    std::string buildScanReport(const ReportContext& context,
                                const fc_scan_result& result,
                                bool previewStyle) const;
    std::string buildCleanupReport(const ReportContext& context,
                                   const fc_cleanup_result& cleanup) const;
};

}  // namespace fcasher::services

#endif
