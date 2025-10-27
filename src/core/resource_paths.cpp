#include "core/resource_paths.h"

#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#endif

namespace resource_paths {
namespace fs = std::filesystem;
namespace {

fs::path executable_directory() {
    std::error_code ec;
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return {};
    }
    buffer.resize(length);
    fs::path executable_path(buffer);
    fs::path canonical_path = fs::weakly_canonical(executable_path, ec);
    if (!ec) {
        executable_path = canonical_path;
    }
    return executable_path.parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    fs::path executable_path(buffer.data());
    fs::path canonical_path = fs::weakly_canonical(executable_path, ec);
    if (!ec) {
        executable_path = canonical_path;
    }
    return executable_path.parent_path();
#elif defined(__linux__)
    fs::path executable_path = fs::read_symlink("/proc/self/exe", ec);
    if (ec) {
        return {};
    }
    fs::path canonical_path = fs::weakly_canonical(executable_path, ec);
    if (!ec) {
        executable_path = canonical_path;
    }
    return executable_path.parent_path();
#else
    fs::path cwd = fs::current_path(ec);
    if (ec) {
        return {};
    }
    return cwd;
#endif
}

fs::path search_upwards(const fs::path& start, const std::string& target) {
    if (start.empty()) {
        return {};
    }

    std::error_code ec;
    fs::path directory = start;
    while (true) {
        fs::path candidate = directory / target;
        if (fs::exists(candidate, ec) && !ec) {
            fs::path resolved = fs::weakly_canonical(candidate, ec);
            return ec ? candidate : resolved;
        }

        if (directory == directory.parent_path()) {
            break;
        }
        directory = directory.parent_path();
        if (directory.empty()) {
            break;
        }
    }

    return {};
}

fs::path locate_directory(const std::string& name) {
    const fs::path executable_dir = executable_directory();
    if (!executable_dir.empty()) {
        if (fs::path from_executable = search_upwards(executable_dir, name); !from_executable.empty()) {
            return from_executable;
        }
    }

    std::error_code ec;
    fs::path current_dir = fs::current_path(ec);
    if (!current_dir.empty()) {
        if (fs::path from_cwd = search_upwards(current_dir, name); !from_cwd.empty()) {
            return from_cwd;
        }
    }

    if (!executable_dir.empty()) {
        return executable_dir / name;
    }

    if (!current_dir.empty()) {
        return current_dir / name;
    }

    return fs::path(name);
}

}  // namespace

fs::path public_directory() {
    return locate_directory("public");
}

fs::path locales_directory() {
    return locate_directory("locales");
}

std::string file_uri_from_path(const fs::path& path) {
    std::string uri = "file://";
    std::string generic_path = path.generic_string();
#if defined(_WIN32)
    if (!generic_path.empty() && generic_path.front() != '/') {
        uri.push_back('/');
    }
#endif
    uri += generic_path;
    if (!uri.empty() && uri.back() != '/') {
        uri.push_back('/');
    }
    return uri;
}

}  // namespace resource_paths
