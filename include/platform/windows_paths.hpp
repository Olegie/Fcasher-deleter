#ifndef FCASHER_PLATFORM_WINDOWS_PATHS_HPP
#define FCASHER_PLATFORM_WINDOWS_PATHS_HPP

#include <string>
#include <vector>

namespace fcasher::platform {

class WindowsPaths {
public:
    WindowsPaths();

    const std::string& userTemp() const;
    const std::string& localAppData() const;
    const std::string& windowsDir() const;
    const std::string& programData() const;
    const std::string& userProfile() const;
    const std::string& programFiles() const;
    const std::string& programFilesX86() const;

    std::vector<std::string> localAppTempRoots() const;
    std::vector<std::string> logRoots() const;
    std::vector<std::string> thumbnailCacheRoots() const;
    std::vector<std::string> shaderCacheRoots() const;
    std::vector<std::string> browserCacheRoots() const;
    std::vector<std::string> crashDumpRoots() const;
    std::vector<std::string> recycleBinRoots() const;

private:
    std::string userTemp_;
    std::string localAppData_;
    std::string windowsDir_;
    std::string programData_;
    std::string userProfile_;
    std::string programFiles_;
    std::string programFilesX86_;
};

}  // namespace fcasher::platform

#endif
