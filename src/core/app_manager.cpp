#include "core/app_manager.h"

#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <libintl.h>
#include <sstream>

#include "ui/html_renderer.h"

namespace {
constexpr char kTextDomain[] = "beaverkiosk";

const char* locale_for_language(Language language) {
    switch (language) {
        case Language::French:
            return "fr";
        case Language::English:
        default:
            return "en";
    }
}

const char* language_env(Language language) {
    switch (language) {
        case Language::French:
            return "fr";
        case Language::English:
        default:
            return "en";
    }
}

class LocaleGuard {
public:
    explicit LocaleGuard(Language language) {
        const char* current = std::setlocale(LC_ALL, nullptr);
        if (current != nullptr) {
            previous_locale_ = current;
        }

        const char* language_env_value = std::getenv("LANGUAGE");
        if (language_env_value != nullptr) {
            previous_language_env_ = language_env_value;
            had_language_env_ = true;
        }

        apply_language(language);
    }

    ~LocaleGuard() {
        if (!previous_locale_.empty()) {
            std::setlocale(LC_ALL, previous_locale_.c_str());
        }

        if (had_language_env_) {
            setenv("LANGUAGE", previous_language_env_.c_str(), 1);
        } else {
            unsetenv("LANGUAGE");
        }
    }

private:
    static void apply_language(Language language) {
        const char* candidates[] = {
            language == Language::French ? "fr_FR.UTF-8" : "en_US.UTF-8",
            language == Language::French ? "fr_FR.utf8"  : "en_US.utf8",
            language == Language::French ? "fr_FR"       : "en_US",
            language == Language::French ? "fr"          : "en",
            "C",
        };

        for (const char* candidate : candidates) {
            if (std::setlocale(LC_ALL, candidate) != nullptr) {
                break;
            }
        }

        setenv("LANGUAGE", language == Language::French ? "fr" : "en", 1);
    }

    std::string previous_locale_;
    std::string previous_language_env_;
    bool had_language_env_ = false;
};

}  // namespace

AppManager::AppManager()
    : apps_({
          {"BeaverPhone", "violet", "icons/phone.svg"},
          {"BeaverSystem", "cyan", "icons/server.svg"},
          {"BeaverAlarm", "amber", "icons/shield-alert.svg"},
          {"BeaverTask", "red", "icons/square-check-big.svg"},
          {"BeaverDoc", "green", "icons/file-text.svg"},
          {"BeaverDebian", "violet", "icons/server-cog.svg"},
          {"BeaverNet", "amber", "icons/chromium.svg"}}),
      default_language_(Language::French) {
    namespace fs = std::filesystem;
    const std::string locale_dir = (fs::current_path() / "locales").string();
    bindtextdomain(kTextDomain, locale_dir.c_str());
    bind_textdomain_codeset(kTextDomain, "UTF-8");
    textdomain(kTextDomain);
}

const std::vector<AppTile>& AppManager::get_available_apps() const {
    return apps_;
}

void AppManager::set_default_language(Language language) {
    default_language_ = language;
}

Language AppManager::get_default_language() const {
    return default_language_;
}

std::string AppManager::to_json() const {
    return to_json(default_language_);
}

std::string AppManager::to_html() const {
    return to_html(default_language_);
}

std::string AppManager::to_json(Language language) const {
    LocaleGuard guard(language);

    std::ostringstream json;
    json << "{\n";
    json << "  \"apps\": [\n";

    for (std::size_t i = 0; i < apps_.size(); ++i) {
        const auto& app = apps_[i];
        const char* localized_name = gettext(app.name.c_str());
        json << "    {\n";
        json << "      \"name\": \"" << localized_name << "\",\n";
        json << "      \"accent\": \"" << app.accent << "\",\n";
        json << "      \"icon\": \"" << app.icon << "\"\n";
        json << "    }";
        if (i + 1 < apps_.size()) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";

    return json.str();
}

std::string AppManager::to_html(Language language) const {
    LocaleGuard guard(language);
    return generate_menu_page_html(apps_, language);
}
