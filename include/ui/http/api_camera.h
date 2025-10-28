#pragma once

#include <map>
#include <string>

#include "ui/http/http_utils.h"

bool handle_camera_api(const std::string& path, const HttpRequest& request,
                       const std::map<std::string, std::string>& query_parameters,
                       HttpResponse& response);

