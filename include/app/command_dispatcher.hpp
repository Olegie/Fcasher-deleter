#ifndef FCASHER_APP_COMMAND_DISPATCHER_HPP
#define FCASHER_APP_COMMAND_DISPATCHER_HPP

#include "app/category_registry.hpp"
#include "app/cli.hpp"
#include "app/safety_policy.hpp"
#include "platform/windows_paths.hpp"
#include "services/export_service.hpp"
#include "services/report_service.hpp"

namespace fcasher::app {

class CommandDispatcher {
public:
    CommandDispatcher();
    int run(const CliOptions& options);

private:
    platform::WindowsPaths paths_;
    CategoryRegistry registry_;
    SafetyPolicy safety_;
    services::ReportService reportService_;
    services::ExportService exportService_;
};

}  // namespace fcasher::app

#endif
