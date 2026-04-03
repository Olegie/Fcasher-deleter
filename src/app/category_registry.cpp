#include "app/category_registry.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

#include "platform/windows_paths.hpp"

namespace fcasher::app {
namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::vector<std::string> uniqueStrings(const std::vector<std::string>& values) {
    std::vector<std::string> result;
    for (const auto& value : values) {
        if (value.empty()) {
            continue;
        }

        const auto it = std::find_if(result.begin(), result.end(), [&](const std::string& existing) {
            return toLower(existing) == toLower(value);
        });
        if (it == result.end()) {
            result.push_back(value);
        }
    }
    return result;
}

CategoryDefinition buildDefinition(fc_category id,
                                   std::string key,
                                   std::string displayName,
                                   std::string description,
                                   std::string reason,
                                   std::vector<std::string> roots,
                                   std::vector<std::string> suffixes,
                                   std::vector<std::string> tokens,
                                   bool recursive,
                                   bool includeAllFiles,
                                   uint32_t modifiedWithinDays,
                                   bool includedInAll,
                                   bool explicitOnly) {
    CategoryDefinition definition;
    definition.id = id;
    definition.key = std::move(key);
    definition.displayName = std::move(displayName);
    definition.description = std::move(description);
    definition.removableReason = std::move(reason);
    definition.roots = uniqueStrings(roots);
    definition.suffixes = std::move(suffixes);
    definition.containsTokens = std::move(tokens);
    definition.recursive = recursive;
    definition.includeAllFiles = includeAllFiles;
    definition.modifiedWithinDays = modifiedWithinDays;
    definition.includedInAll = includedInAll;
    definition.explicitOnly = explicitOnly;
    return definition;
}

PresetDefinition buildPreset(std::string key,
                             std::string displayName,
                             std::string description,
                             std::vector<std::string> categoryTokens) {
    PresetDefinition definition;
    definition.key = std::move(key);
    definition.displayName = std::move(displayName);
    definition.description = std::move(description);
    definition.categoryTokens = uniqueStrings(categoryTokens);
    return definition;
}

}  // namespace

CategoryRegistry::CategoryRegistry(const platform::WindowsPaths& paths) {
    categories_.push_back(buildDefinition(
        FC_CATEGORY_USER_TEMP,
        "user-temp",
        "User Temp",
        "Per-user temporary directory rooted at the active TEMP environment path.",
        "Located under the active per-user temp directory.",
        std::vector<std::string>{paths.userTemp()},
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_LOCAL_APP_TEMP,
        "local-app-temp",
        "Local App Temp",
        "Local application temp locations below LocalAppData.",
        "Located in a LocalAppData temp root typically used for disposable application artifacts.",
        paths.localAppTempRoots(),
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_WINDOWS_TEMP,
        "windows-temp",
        "Windows Temp",
        "System-wide Windows temp directory.",
        "Located under the shared Windows temp directory.",
        std::vector<std::string>{paths.windowsDir() + "\\Temp"},
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_LOGS,
        "logs",
        "Logs",
        "Windows and application logs that are safe to inspect before deletion.",
        "Matches log or WER report artifacts from supported log locations.",
        paths.logRoots(),
        std::vector<std::string>{".log", ".etl", ".txt", ".wer"},
        std::vector<std::string>{},
        true,
        false,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_THUMBNAIL_CACHE,
        "thumbnail-cache",
        "Thumbnail Cache",
        "Windows thumbnail and icon cache databases.",
        "Matches thumbnail or icon cache database files.",
        paths.thumbnailCacheRoots(),
        std::vector<std::string>{},
        std::vector<std::string>{"thumbcache_", "iconcache_"},
        true,
        false,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_SHADER_CACHE,
        "shader-cache",
        "Shader Cache",
        "Graphics shader caches for Direct3D and vendor-specific drivers.",
        "Located inside a known shader cache directory.",
        paths.shaderCacheRoots(),
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_BROWSER_CACHE,
        "browser-cache",
        "Browser Cache",
        "Supported browser HTTP, code, and temporary web caches.",
        "Located inside a known browser cache directory.",
        paths.browserCacheRoots(),
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_CRASH_DUMPS,
        "crash-dumps",
        "Crash Dumps",
        "Crash dump and WER dump directories.",
        "Matches crash dump artifacts from supported dump directories.",
        paths.crashDumpRoots(),
        std::vector<std::string>{".dmp", ".mdmp", ".wer"},
        std::vector<std::string>{},
        true,
        false,
        0,
        true,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_RECENT_ARTIFACTS,
        "recent-artifacts",
        "Recent Artifacts",
        "Recently modified temporary artifacts for quick review.",
        "Matches recently modified temporary artifacts in temp locations.",
        uniqueStrings({paths.userTemp(), paths.localAppData() + "\\Temp"}),
        std::vector<std::string>{".tmp", ".temp", ".log", ".etl", ".dmp", ".old", ".bak"},
        std::vector<std::string>{},
        true,
        false,
        7,
        false,
        false));

    categories_.push_back(buildDefinition(
        FC_CATEGORY_RECYCLE_BIN,
        "recycle-bin",
        "Recycle Bin",
        "Recycle bin roots. Never included in --all and always explicit.",
        "Located inside an explicitly requested recycle bin root.",
        paths.recycleBinRoots(),
        std::vector<std::string>{},
        std::vector<std::string>{},
        true,
        true,
        0,
        false,
        true));

    presets_.push_back(buildPreset("quick-sweep",
                                   "Quick Sweep",
                                   "High-value daily sweep across temp, recent artifacts, and logs.",
                                   {"temp", "recent-artifacts", "logs"}));
    presets_.push_back(buildPreset("browser-focus",
                                   "Browser Focus",
                                   "Browser cache and explorer thumbnail cleanup for UI-heavy desktops.",
                                   {"browser-cache", "thumbnail-cache"}));
    presets_.push_back(buildPreset("graphics-stack",
                                   "Graphics Stack",
                                   "Shader and thumbnail caches typically regenerated by the graphics stack.",
                                   {"shader-cache", "thumbnail-cache"}));
    presets_.push_back(buildPreset("diagnostics",
                                   "Diagnostics",
                                   "Logs, crash dumps, and recent artifacts useful during troubleshooting.",
                                   {"logs", "crash-dumps", "recent-artifacts"}));
    presets_.push_back(buildPreset("safe-all",
                                   "Safe All",
                                   "Equivalent to --all using every non-explicit built-in cleanup category.",
                                   {"temp", "logs", "browser-cache", "thumbnail-cache", "shader-cache",
                                    "crash-dumps"}));
}

const std::vector<CategoryDefinition>& CategoryRegistry::all() const {
    return categories_;
}

const std::vector<PresetDefinition>& CategoryRegistry::presets() const {
    return presets_;
}

const CategoryDefinition* CategoryRegistry::findByKey(const std::string& key) const {
    const std::string lowered = toLower(key);
    const auto it = std::find_if(categories_.begin(), categories_.end(), [&](const CategoryDefinition& definition) {
        return toLower(definition.key) == lowered;
    });
    return it == categories_.end() ? nullptr : &(*it);
}

const PresetDefinition* CategoryRegistry::findPresetByKey(const std::string& key) const {
    const std::string lowered = toLower(key);
    const auto it = std::find_if(presets_.begin(), presets_.end(), [&](const PresetDefinition& definition) {
        return toLower(definition.key) == lowered;
    });
    return it == presets_.end() ? nullptr : &(*it);
}

std::vector<std::string> CategoryRegistry::expandPresetTokens(const std::vector<std::string>& presetKeys,
                                                              std::vector<std::string>* warnings) const {
    std::vector<std::string> tokens;

    for (const auto& rawPreset : presetKeys) {
        const PresetDefinition* preset = findPresetByKey(rawPreset);
        if (preset == nullptr) {
            if (warnings != nullptr) {
                warnings->push_back("Unknown preset ignored: " + rawPreset);
            }
            continue;
        }

        for (const auto& token : preset->categoryTokens) {
            const auto duplicate = std::find_if(tokens.begin(), tokens.end(), [&](const std::string& existing) {
                return toLower(existing) == toLower(token);
            });
            if (duplicate == tokens.end()) {
                tokens.push_back(token);
            }
        }
    }

    return tokens;
}

std::vector<const CategoryDefinition*> CategoryRegistry::resolve(const std::vector<std::string>& tokens,
                                                                 bool selectAll,
                                                                 std::vector<std::string>* warnings) const {
    std::vector<const CategoryDefinition*> selected;

    const auto addCategory = [&](fc_category id) {
        const auto it = std::find_if(categories_.begin(), categories_.end(), [&](const CategoryDefinition& definition) {
            return definition.id == id;
        });
        if (it == categories_.end()) {
            return;
        }

        const auto duplicate = std::find(selected.begin(), selected.end(), &(*it));
        if (duplicate == selected.end()) {
            selected.push_back(&(*it));
        }
    };

    if (selectAll) {
        for (const auto& category : categories_) {
            if (category.includedInAll && !category.explicitOnly) {
                addCategory(category.id);
            }
        }
    }

    for (const auto& rawToken : tokens) {
        const std::string token = toLower(rawToken);
        if (token == "temp") {
            addCategory(FC_CATEGORY_USER_TEMP);
            addCategory(FC_CATEGORY_LOCAL_APP_TEMP);
            addCategory(FC_CATEGORY_WINDOWS_TEMP);
        } else if (token == "logs") {
            addCategory(FC_CATEGORY_LOGS);
        } else if (token == "browser" || token == "browser-cache") {
            addCategory(FC_CATEGORY_BROWSER_CACHE);
        } else if (token == "thumbnails" || token == "thumbnail-cache") {
            addCategory(FC_CATEGORY_THUMBNAIL_CACHE);
        } else if (token == "shader" || token == "shader-cache") {
            addCategory(FC_CATEGORY_SHADER_CACHE);
        } else if (token == "crash-dumps" || token == "dumps") {
            addCategory(FC_CATEGORY_CRASH_DUMPS);
        } else if (token == "recent" || token == "recent-artifacts") {
            addCategory(FC_CATEGORY_RECENT_ARTIFACTS);
        } else if (token == "recycle-bin" || token == "recycle") {
            addCategory(FC_CATEGORY_RECYCLE_BIN);
        } else if (const CategoryDefinition* definition = findByKey(token)) {
            addCategory(definition->id);
        } else if (warnings != nullptr) {
            warnings->push_back("Unknown category token ignored: " + rawToken);
        }
    }

    return selected;
}

}  // namespace fcasher::app
