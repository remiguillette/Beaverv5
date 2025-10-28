#pragma once

#include <string>
#include <vector>

#include "core/app_manager.h"

class TranslationCatalog;

std::string generate_app_tile_html(const AppTile& app, const TranslationCatalog& translations,
                                   Language language, MenuRouteMode route_mode,
                                   const std::string& asset_prefix = "");
std::string generate_menu_page_html(const std::vector<AppTile>& apps,
                                    const TranslationCatalog& translations, Language language,
                                    MenuRouteMode route_mode,
                                    const std::string& asset_prefix = "");
std::string generate_beaverphone_dialpad_html(
    const TranslationCatalog& translations, Language language,
    const std::string& asset_prefix = "",
    BeaverphoneMenuLinkMode menu_link_mode = BeaverphoneMenuLinkMode::kAbsoluteRoot);
