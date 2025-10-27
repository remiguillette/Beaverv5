#pragma once

#include <filesystem>
#include <string>

namespace resource_paths {

std::filesystem::path public_directory();
std::filesystem::path locales_directory();
std::string file_uri_from_path(const std::filesystem::path& path);

}  // namespace resource_paths
