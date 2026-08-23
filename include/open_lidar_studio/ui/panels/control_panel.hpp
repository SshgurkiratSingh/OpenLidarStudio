#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/hardware/lidar_controller.hpp>
#include <open_lidar_studio/pipeline/signal_filter.hpp>
#include <open_lidar_studio/pipeline/auto_ranger.hpp>

namespace ols::ui {

class ControlPanel {
public:
    ControlPanel();
    ~ControlPanel() = default;

    void render(UIState& state, hardware::LidarController& controller, pipeline::SignalFilter& filter, pipeline::AutoRanger& ranger);
    
private:
    void refreshPorts(UIState& state);
    std::vector<std::string> ports_;
};

} // namespace ols::ui
