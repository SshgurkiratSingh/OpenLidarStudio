#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <open_lidar_studio/core/telemetry_types.hpp>

namespace ols::ui {

struct TrackedObject {
    int id;
    float center_x;
    float center_y;
    float radius;
    float velocity_x;
    float velocity_y;
    int missing_frames;
    int hit_count{0};
    bool is_confirmed{false};
    float initial_x{0.0f};
    float initial_y{0.0f};
};

struct ZoneAlert {
    bool   enabled{false};
    float  min_radius{0.0f};
    float  max_radius{1.0f};
    float  min_angle_deg{0.0f};
    float  max_angle_deg{360.0f};
    bool   play_sound{false};
    bool   triggered{false};
    float  trigger_flash{0.0f}; // countdown timer for red flash
};

struct UIState {
    // ── Connection ───────────────────────────────────────────────────
    std::string selected_port{"/dev/ttyUSB0"};
    int selected_baudrate{115200};
    int custom_baudrate{115200};
    bool single_channel{true};
    bool is_connected{false};
    bool is_scanning{false};

    // ── LiDAR Parameters ─────────────────────────────────────────────
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
    float point_size{3.0f};       // OpenGL point size
    bool circular_points{true};   // Circular vs square points
    bool show_grid{true};         // Cartesian grid overlay
    float grid_spacing_m{0.5f};   // Grid line every N metres

    // ── Viewport Zoom/Pan ─────────────────────────────────────────────
    float zoom{1.0f};             // 1.0 = fit-to-range; <1 zoomed out, >1 zoomed in
    float pan_x{0.0f};            // Pan offset in metres (world-space)
    float pan_y{0.0f};

    // ── Measurement Tool ─────────────────────────────────────────────
    bool measure_mode{false};
    bool measure_point_a_set{false};
    bool measure_point_b_set{false};
    float measure_ax{0.0f}, measure_ay{0.0f}; // world-space metres
    float measure_bx{0.0f}, measure_by{0.0f};

    // ── Zone Alerts ───────────────────────────────────────────────────
    static constexpr int NUM_ZONES = 4;
    std::array<ZoneAlert, NUM_ZONES> zones{};

    // ── Screenshot ────────────────────────────────────────────────────
    bool request_screenshot{false};
    std::string screenshot_filename{"screenshot"};

    // ── Telemetry readouts (read-only) ────────────────────────────────
    float current_rpm{0.0f};
    float r_inst{0.0f};
    float r_smooth{0.0f};
    float alpha_adaptive{0.05f};
    float r_final{10.0f};
    size_t point_count{0};

    // ── History for plots ─────────────────────────────────────────────
    std::vector<float> r_inst_history;
    std::vector<float> r_smooth_history;
    std::vector<float> alpha_history;
    std::vector<float> point_history;
    std::vector<float> time_history;
    float current_time{0.0f};
    
    // ── Logging ───────────────────────────────────────────────────────
    bool is_logging{false};
    std::string log_filename{"lidar_capture"};
    float log_duration{0.0f};

    // ── Person Tracking ───────────────────────────────────────────────
    bool enable_person_tracking{false};
    float track_cluster_dist{0.2f};
    float track_min_size{0.1f};
    float track_max_size{0.8f};
    int track_min_hits{3};
    std::vector<TrackedObject> tracked_objects;
};

} // namespace ols::ui
