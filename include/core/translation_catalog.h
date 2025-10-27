#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "core/language.h"

class TranslationCatalog {
public:
    explicit TranslationCatalog(std::string locales_directory);

    std::string translate(const std::string& key, Language language) const;

private:
    using TranslationMap = std::unordered_map<std::string, std::string>;

    static TranslationMap load_language_file(const std::string& locales_directory,
                                             const std::string& language_code);
    static std::string trim(const std::string& text);

    std::map<Language, TranslationMap> translations_;
};
