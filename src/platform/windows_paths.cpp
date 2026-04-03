#include "platform/windows_paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

#include <windows.h>

namespace fcasher::platform {
namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '/', '\\');
    while (value.size() > 3 && !value.empty() && value.back() == '\\') {
        value.pop_back();
    }
    return value;
}

std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return normalizePath(right);
    }
    if (right.empty()) {
        return normalizePath(left);
    }

    if (left.back() == '\\' || left.back() == '/') {
        return normalizePath(left + right);
    }
    return normalizePath(left + "\\" + right);
}

std::string environmentValue(const char* name) {
    const DWORD required = GetEnvironmentVariableA(name, nullptr, 0);
    if (required == 0) {
        return {};
    }

    std::string value(required, '\0');
    const DWORD written = GetEnvironmentVariableA(name, value.data(), required);
    if (written == 0) {
        return {};
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return normalizePath(value);
}

std::string currentDirectory() {
    char buffer[MAX_PATH] = {};
    const DWORD written = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (written == 0 || written >= MAX_PATH) {
        return "C:\\";
    }
    return normalizePath(buffer);
}

std::string resolveWindowsDirectory() {
    std::string value = environmentValue("WINDIR");
    if (!value.empty()) {
        return value;
    }

    char buffer[MAX_PATH] = {};
    const UINT written = GetWindowsDirectoryA(buffer, MAX_PATH);
    if (written == 0 || written >= MAX_PATH) {
        return "C:\\Windows";
    }
    return normalizePath(buffer);
}

std::string resolveUserProfile() {
    std::string profile = environmentValue("USERPROFILE");
    if (!profile.empty()) {
        return profile;
    }

    const std::string drive = environmentValue("HOMEDRIVE");
    const std::string path = environmentValue("HOMEPATH");
    if (!drive.empty() || !path.empty()) {
        return normalizePath(drive + path);
    }

    return currentDirectory();
}

std::string resolveLocalAppData(const std::string& userProfile) {
    std::string value = environmentValue("LOCALAPPDATA");
    if (!value.empty()) {
        return value;
    }

    const std::string modern = joinPath(userProfile, "AppData\\Local");
    if (GetFileAttributesA(modern.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return modern;
    }

    return joinPath(userProfile, "Local Settings\\Application Data");
}

std::string resolveProgramData() {
    std::string value = environmentValue("PROGRAMDATA");
    if (!value.empty()) {
        return value;
    }

    value = environmentValue("ALLUSERSPROFILE");
    if (!value.empty()) {
        return value;
    }

    return "C:\\ProgramData";
}

std::vector<std::string> uniquePaths(const std::vector<std::string>& input) {
    std::vector<std::string> result;
    for (const auto& value : input) {
        if (value.empty()) {
            continue;
        }

        const std::string normalized = normalizePath(value);
        const std::string lowered = toLower(normalized);
        const bool exists = std::any_of(result.begin(), result.end(), [&](const std::string& existing) {
            return toLower(existing) == lowered;
        });
        if (!exists) {
            result.push_back(normalized);
        }
    }
    return result;
}

std::vector<std::string> fixedDriveRoots() {
    char buffer[512] = {};
    const DWORD written = GetLogicalDriveStringsA(sizeof(buffer), buffer);
    std::vector<std::string> drives;

    if (written == 0 || written >= sizeof(buffer)) {
        drives.push_back("C:\\");
        return drives;
    }

    const char* current = buffer;
    while (*current != '\0') {
        if (GetDriveTypeA(current) == DRIVE_FIXED) {
            drives.emplace_back(normalizePath(current));
        }
        current += std::strlen(current) + 1;
    }

    if (drives.empty()) {
        drives.push_back("C:\\");
    }

    return drives;
}

}  // namespace

WindowsPaths::WindowsPaths()
    : userTemp_(normalizePath(environmentValue("TEMP"))),
      localAppData_(resolveLocalAppData(resolveUserProfile())),
      windowsDir_(resolveWindowsDirectory()),
      programData_(resolveProgramData()),
      userProfile_(resolveUserProfile()),
      programFiles_(environmentValue("ProgramFiles")),
      programFilesX86_(environmentValue("ProgramFiles(x86)")) {
    if (userTemp_.empty()) {
        userTemp_ = normalizePath(environmentValue("TMP"));
    }
    if (userTemp_.empty()) {
        userTemp_ = joinPath(localAppData_, "Temp");
    }

    if (programFiles_.empty()) {
        programFiles_ = "C:\\Program Files";
    }
    if (programFilesX86_.empty()) {
        programFilesX86_ = programFiles_;
    }
}

const std::string& WindowsPaths::userTemp() const {
    return userTemp_;
}

const std::string& WindowsPaths::localAppData() const {
    return localAppData_;
}

const std::string& WindowsPaths::windowsDir() const {
    return windowsDir_;
}

const std::string& WindowsPaths::programData() const {
    return programData_;
}

const std::string& WindowsPaths::userProfile() const {
    return userProfile_;
}

const std::string& WindowsPaths::programFiles() const {
    return programFiles_;
}

const std::string& WindowsPaths::programFilesX86() const {
    return programFilesX86_;
}

std::vector<std::string> WindowsPaths::localAppTempRoots() const {
    return uniquePaths({
        joinPath(localAppData_, "Temp"),
        joinPath(localAppData_, "Temp\\Low")
    });
}

std::vector<std::string> WindowsPaths::logRoots() const {
    return uniquePaths({
        joinPath(windowsDir_, "Logs"),
        joinPath(programData_, "Microsoft\\Windows\\WER\\ReportQueue"),
        joinPath(programData_, "Microsoft\\Windows\\WER\\ReportArchive"),
        joinPath(localAppData_, "Diagnostics")
    });
}

std::vector<std::string> WindowsPaths::thumbnailCacheRoots() const {
    return uniquePaths({
        joinPath(localAppData_, "Microsoft\\Windows\\Explorer")
    });
}

std::vector<std::string> WindowsPaths::shaderCacheRoots() const {
    return uniquePaths({
        joinPath(localAppData_, "D3DSCache"),
        joinPath(localAppData_, "NVIDIA\\DXCache"),
        joinPath(localAppData_, "NVIDIA\\GLCache"),
        joinPath(localAppData_, "AMD\\DxCache"),
        joinPath(localAppData_, "AMD\\GLCache")
    });
}

std::vector<std::string> WindowsPaths::browserCacheRoots() const {
    return uniquePaths({
        joinPath(localAppData_, "Microsoft\\Windows\\INetCache"),
        joinPath(userProfile_, "Local Settings\\Temporary Internet Files"),
        joinPath(localAppData_, "Google\\Chrome\\User Data\\Default\\Cache"),
        joinPath(localAppData_, "Google\\Chrome\\User Data\\Default\\Code Cache"),
        joinPath(localAppData_, "Microsoft\\Edge\\User Data\\Default\\Cache"),
        joinPath(localAppData_, "Microsoft\\Edge\\User Data\\Default\\Code Cache"),
        joinPath(localAppData_, "BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache"),
        joinPath(localAppData_, "Opera Software\\Opera Stable\\Cache")
    });
}

std::vector<std::string> WindowsPaths::crashDumpRoots() const {
    return uniquePaths({
        joinPath(localAppData_, "CrashDumps"),
        joinPath(windowsDir_, "Minidump"),
        joinPath(windowsDir_, "LiveKernelReports")
    });
}

std::vector<std::string> WindowsPaths::recycleBinRoots() const {
    std::vector<std::string> roots;
    for (const auto& drive : fixedDriveRoots()) {
        roots.push_back(joinPath(drive, "$Recycle.Bin"));
        roots.push_back(joinPath(drive, "RECYCLER"));
        roots.push_back(joinPath(drive, "Recycler"));
    }
    return uniquePaths(roots);
}

}  // namespace fcasher::platform
