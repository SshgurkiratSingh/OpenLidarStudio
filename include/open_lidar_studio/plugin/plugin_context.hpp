#pragma once

#include <vector>
#include <CYdLidar.h>
#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/rendering/gl_renderer.hpp>
#include <open_lidar_studio/hardware/lidar_controller.hpp>
#include <open_lidar_studio/core/audio_manager.hpp>

namespace ols::plugin {

struct PluginContext {
    ui::UIState* state{nullptr};
    const std::vector<LaserPoint>* raw_points{nullptr};
    rendering::GLRenderer* renderer{nullptr};
    hardware::LidarController* lidar{nullptr};
    core::AudioManager* audio{nullptr};
};

} // namespace ols::plugin
