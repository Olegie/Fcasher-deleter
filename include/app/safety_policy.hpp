#ifndef FCASHER_APP_SAFETY_POLICY_HPP
#define FCASHER_APP_SAFETY_POLICY_HPP

#include <string>
#include <string_view>
#include <vector>

#include "core/file_record.h"

namespace fcasher::platform {
class WindowsPaths;
}

namespace fcasher::app {

class SafetyPolicy {
public:
    explicit SafetyPolicy(const platform::WindowsPaths& paths);

    const std::vector<std::string>& protectedRoots() const;
    bool isProtectedPath(std::string_view path) const;
    bool isDeletionAllowed(const fc_file_record& record, std::string* reason) const;

private:
    std::vector<std::string> protectedRoots_;
};

}  // namespace fcasher::app

#endif
