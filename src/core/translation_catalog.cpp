#include "core/translation_catalog.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <utility>

TranslationCatalog::TranslationCatalog(std::string locales_directory)
    : translations_({{Language::English, load_language_file(locales_directory, "en")},
                     {Language::French, load_language_file(locales_directory, "fr")}}) {}

std::string TranslationCatalog::translate(const std::string& key, Language language) const {
    const auto language_it = translations_.find(language);
    if (language_it != translations_.end()) {
        const auto& language_map = language_it->second;
        const auto translation_it = language_map.find(key);
        if (translation_it != language_map.end()) {
            return translation_it->second;
        }
    }

    const auto english_it = translations_.find(Language::English);
    if (english_it != translations_.end()) {
        const auto& english_map = english_it->second;
        const auto translation_it = english_map.find(key);
        if (translation_it != english_map.end()) {
            return translation_it->second;
        }
    }

    return key;
}

TranslationCatalog::TranslationMap TranslationCatalog::load_language_file(
    const std::string& locales_directory, const std::string& language_code) {
    TranslationMap translations;

    namespace fs = std::filesystem;
    const fs::path file_path = fs::path(locales_directory) / language_code / "strings.txt";
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return translations;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::size_t comment_position = line.find('#');
        if (comment_position != std::string::npos) {
            line = line.substr(0, comment_position);
        }

        const std::size_t separator_position = line.find('=');
        if (separator_position == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, separator_position));
        std::string value = trim(line.substr(separator_position + 1));
        if (!key.empty()) {
            translations[std::move(key)] = std::move(value);
        }
    }

    return translations;
}

std::string TranslationCatalog::trim(const std::string& text) {
    auto begin = text.begin();
    auto end = text.end();

    while (begin != end && std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }

    while (begin != end) {
        auto last = end;
        --last;
        if (std::isspace(static_cast<unsigned char>(*last)) == 0) {
            break;
        }
        end = last;
    }

    return std::string(begin, end);
}
