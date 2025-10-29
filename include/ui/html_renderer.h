#pragma once

#include <string>
#include <vector>

#include "core/app_manager.h"
#include "core/system_status.h"

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
std::string generate_beaveralarm_console_html(
    const TranslationCatalog& translations, Language language,
    const std::string& asset_prefix = "",
    BeaverAlarmMenuLinkMode menu_link_mode = BeaverAlarmMenuLinkMode::kAbsoluteRoot);
std::string generate_beaversystem_dashboard_html(
    const TranslationCatalog& translations, Language language, const std::string& asset_prefix = "",
    BeaverSystemMenuLinkMode menu_link_mode = BeaverSystemMenuLinkMode::kAbsoluteRoot,
    const SystemStatusSnapshot& snapshot = SystemStatusSnapshot{});
std::string generate_beavertask_board_html(
    const TranslationCatalog& translations, Language language, const std::string& asset_prefix = "",
    BeaverTaskMenuLinkMode menu_link_mode = BeaverTaskMenuLinkMode::kAbsoluteRoot);
