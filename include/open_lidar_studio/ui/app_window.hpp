#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/ui/panels/viewport_panel.hpp>
#include <open_lidar_studio/ui/panels/control_panel.hpp>
#include <open_lidar_studio/ui/panels/telemetry_panel.hpp>
#include <open_lidar_studio/ui/panels/logging_panel.hpp>
#include <open_lidar_studio/ui/panels/replay_panel.hpp>
#include <open_lidar_studio/hardware/lidar_controller.hpp>
#include <deque>
#include <open_lidar_studio/pipeline/signal_filter.hpp>
#include <open_lidar_studio/pipeline/auto_ranger.hpp>
#include <open_lidar_studio/pipeline/async_logger.hpp>
#include <open_lidar_studio/rendering/gl_renderer.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <mutex>

namespace ols::ui {

class AppWindow {
public:
    AppWindow(int width, int height, const char* title);
    ~AppWindow();

    bool initialize();
    void run();

private:
    void renderUI();
    void setupDockspace();
    void processScanData();

    GLFWwindow* window_{nullptr};
    int width_;
    int height_;
    const char* title_;

    UIState state_;
    ViewportPanel viewport_panel_;
    ControlPanel control_panel_;
    TelemetryPanel telemetry_panel_;
    LoggingPanel logging_panel_;
    ReplayPanel replay_panel_;

    hardware::LidarController lidar_controller_;
    pipeline::SignalFilter signal_filter_;
    pipeline::AutoRanger auto_ranger_;
    pipeline::AsyncLogger async_logger_;
    rendering::GLRenderer gl_renderer_;
    
    // Concurrency between lidar thread and UI thread
    std::mutex scan_mutex_;
    std::vector<LaserPoint> latest_scan_;
    bool new_scan_available_{false};
    
    std::deque<std::vector<core::SerializedPoint>> history_frames_;
};

} // namespace ols::ui
