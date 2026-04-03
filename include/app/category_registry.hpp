#ifndef FCASHER_APP_CATEGORY_REGISTRY_HPP
#define FCASHER_APP_CATEGORY_REGISTRY_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "core/file_record.h"

namespace fcasher::platform {
class WindowsPaths;
}

namespace fcasher::app {

struct CategoryDefinition {
    fc_category id{FC_CATEGORY_UNKNOWN};
    std::string key;
    std::string displayName;
    std::string description;
    std::string removableReason;
    std::vector<std::string> roots;
    std::vector<std::string> suffixes;
    std::vector<std::string> containsTokens;
    bool recursive{true};
    bool includeAllFiles{true};
    uint32_t modifiedWithinDays{0};
    bool includedInAll{true};
    bool explicitOnly{false};
};

struct PresetDefinition {
    std::string key;
    std::string displayName;
    std::string description;
    std::vector<std::string> categoryTokens;
};

class CategoryRegistry {
public:
    explicit CategoryRegistry(const platform::WindowsPaths& paths);

    const std::vector<CategoryDefinition>& all() const;
    const std::vector<PresetDefinition>& presets() const;
    std::vector<const CategoryDefinition*> resolve(const std::vector<std::string>& tokens,
                                                   bool selectAll,
                                                   std::vector<std::string>* warnings) const;
    const CategoryDefinition* findByKey(const std::string& key) const;
    const PresetDefinition* findPresetByKey(const std::string& key) const;
    std::vector<std::string> expandPresetTokens(const std::vector<std::string>& presetKeys,
                                                std::vector<std::string>* warnings) const;

private:
    std::vector<CategoryDefinition> categories_;
    std::vector<PresetDefinition> presets_;
};

}  // namespace fcasher::app

#endif
