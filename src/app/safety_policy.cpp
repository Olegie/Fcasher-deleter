#include "app/safety_policy.hpp"

#include <algorithm>
#include <cctype>

#include "platform/windows_paths.hpp"

namespace fcasher::app {
namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool pathStartsWith(std::string_view path, std::string_view root) {
    const std::string loweredPath = toLower(std::string(path));
    const std::string loweredRoot = toLower(std::string(root));

    if (loweredPath.size() < loweredRoot.size()) {
        return false;
    }
    if (loweredPath.compare(0, loweredRoot.size(), loweredRoot) != 0) {
        return false;
    }
    if (loweredPath.size() == loweredRoot.size()) {
        return true;
    }
    return loweredRoot.back() == '\\' || loweredPath[loweredRoot.size()] == '\\';
}

std::vector<std::string> uniqueRoots(const std::vector<std::string>& roots) {
    std::vector<std::string> result;
    for (const auto& root : roots) {
        if (root.empty()) {
            continue;
        }
        const auto duplicate = std::find_if(result.begin(), result.end(), [&](const std::string& existing) {
            return toLower(existing) == toLower(root);
        });
        if (duplicate == result.end()) {
            result.push_back(root);
        }
    }
    return result;
}

}  // namespace

SafetyPolicy::SafetyPolicy(const platform::WindowsPaths& paths)
    : protectedRoots_(uniqueRoots({
          paths.windowsDir() + "\\System32",
          paths.windowsDir() + "\\WinSxS",
          paths.windowsDir() + "\\Boot",
          paths.programFiles(),
          paths.programFilesX86(),
          paths.userProfile() + "\\Documents",
          paths.userProfile() + "\\Desktop",
          paths.userProfile() + "\\Pictures",
          paths.userProfile() + "\\Videos",
          paths.userProfile() + "\\Music",
          paths.userProfile() + "\\Downloads"
      })) {}

const std::vector<std::string>& SafetyPolicy::protectedRoots() const {
    return protectedRoots_;
}

bool SafetyPolicy::isProtectedPath(std::string_view path) const {
    return std::any_of(protectedRoots_.begin(), protectedRoots_.end(), [&](const std::string& root) {
        return pathStartsWith(path, root);
    });
}

bool SafetyPolicy::isDeletionAllowed(const fc_file_record& record, std::string* reason) const {
    if (record.category == FC_CATEGORY_UNKNOWN) {
        if (reason != nullptr) {
            *reason = "unknown category";
        }
        return false;
    }

    if (isProtectedPath(record.path)) {
        if (reason != nullptr) {
            *reason = "protected root";
        }
        return false;
    }

    return true;
}

}  // namespace fcasher::app
