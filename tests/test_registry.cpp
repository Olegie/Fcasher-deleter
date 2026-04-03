#include "app/category_registry.hpp"
#include "platform/windows_paths.hpp"
#include "test_common.hpp"

FC_TEST(category_registry_expands_presets) {
    fcasher::platform::WindowsPaths paths;
    fcasher::app::CategoryRegistry registry(paths);

    const auto* preset = registry.findPresetByKey("quick-sweep");
    FC_ASSERT(preset != nullptr);
    FC_ASSERT(!preset->categoryTokens.empty());

    std::vector<std::string> warnings;
    const auto expanded = registry.expandPresetTokens({"quick-sweep"}, &warnings);
    FC_ASSERT(warnings.empty());
    FC_ASSERT(expanded.size() >= 2U);

    const auto* temp = registry.findByKey("user-temp");
    const auto* logs = registry.findByKey("logs");
    FC_ASSERT(temp != nullptr);
    FC_ASSERT(logs != nullptr);
    return true;
}
