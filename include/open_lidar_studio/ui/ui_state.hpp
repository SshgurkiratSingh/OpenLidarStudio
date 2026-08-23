#pragma once

#include <string>
#include <vector>
#include <open_lidar_studio/core/telemetry_types.hpp>

namespace ols::ui {

struct UIState {
    // Toolbar
    std::string selected_port{"/dev/ttyUSB0"};
    int selected_baudrate{115200};
    int custom_baudrate{115200};
    bool single_channel{true};
    bool is_connected{false};
    bool is_scanning{false};

    // Parameter & Control
    float target_rpm{600.0f};
    bool auto_ranging{true};
    float manual_range{10.0f};
    float alpha_min{0.05f};
    float alpha_max{0.40f};
    float k_sensitivity{1.50f};
    bool enable_history{true};
    int history_length{15};
    float history_decay{1.5f};
    float min_angle_deg{0.0f};
    float max_angle_deg{360.0f};
    float min_radius{0.1f};
    float max_radius{100.0f};
    int color_mode{0}; // 0=Range, 1=Intensity

    // Telemetry & State readouts (read-only for UI)
    float current_rpm{0.0f};
    float r_inst{0.0f};
    float r_smooth{0.0f};
    float alpha_adaptive{0.05f};
    float r_final{10.0f};
    size_t point_count{0};

    // History for plots
    std::vector<float> r_inst_history;
    std::vector<float> r_smooth_history;
    std::vector<float> alpha_history;
    std::vector<float> point_history;
    std::vector<float> time_history;
    float current_time{0.0f};
    
    // Logging
    bool is_logging{false};
    std::string log_filename{"lidar_capture"};
    float log_duration{0.0f};
};

} // namespace ols::ui
