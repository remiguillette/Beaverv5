#pragma once

enum class Language {
    French,
    English,
};

inline const char* language_to_string(Language language) {
    switch (language) {
        case Language::French:
            return "French";
        case Language::English:
            return "English";
    }

    return "Unknown";
}
