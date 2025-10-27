#pragma once

#include <string>
#include <vector>

#include "core/app_manager.h"

std::string generate_app_tile_html(const AppTile& app);
std::string generate_menu_page_html(const std::vector<AppTile>& apps, Language language);
