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
    std::vector<std::string> presets;
    std::vector<std::string> activeFilters;
    std::string sortMode{"native"};
    std::size_t sourceFileCount{0};
    uint64_t sourceSizeBytes{0};
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
    std::string buildAnalysisReport(const ReportContext& context,
                                    const fc_scan_result& result) const;
    std::string buildCategoryCatalogReport(const std::vector<fcasher::app::CategoryDefinition>& categories,
                                           const std::vector<fcasher::app::PresetDefinition>& presets) const;
};

}  // namespace fcasher::services

#endif
