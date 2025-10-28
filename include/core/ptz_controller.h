#pragma once

#include <string>

#include "core/cctv_config.h"

struct CommandResult {
    bool success;
    std::string message;

    static CommandResult Ok(std::string msg = {});
    static CommandResult Error(std::string msg);
};

class PtzController {
public:
    explicit PtzController(const CctvConfig& config);

    CommandResult pan_left();
    CommandResult pan_right();
    CommandResult tilt_up();
    CommandResult tilt_down();
    CommandResult zoom_in();
    CommandResult zoom_out();
    CommandResult stop();

private:
    CommandResult send_continuous_move(double pan, double tilt, double zoom);
    CommandResult send_stop(bool pan_tilt, bool zoom);
    CommandResult send_soap_request(const std::string& body, const std::string& soap_action);

    CctvConfig config_;
    double pan_speed_ = 0.3;
    double tilt_speed_ = 0.3;
    double zoom_speed_ = 0.3;
};

