#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/pipeline/async_logger.hpp>

namespace ols::ui {

class LoggingPanel {
public:
    LoggingPanel();
    ~LoggingPanel() = default;

    void render(UIState& state, pipeline::AsyncLogger& logger);
};

} // namespace ols::ui
