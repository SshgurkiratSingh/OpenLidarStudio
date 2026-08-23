#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/core/telemetry_types.hpp>
#include <open_lidar_studio/rendering/gl_renderer.hpp>
#include <vector>
#include <string>

namespace ols::ui {

// Stores one decoded frame from a recording
struct ReplayFrame {
    core::TelemetryHeader header;
    std::vector<core::SerializedPoint> points;
};

class ReplayPanel {
public:
    ReplayPanel();
    ~ReplayPanel() = default;

    void render(UIState& state, rendering::GLRenderer& renderer);

private:
    bool loadFile(const std::string& base);
    void seekToFrame(int idx);

    std::vector<ReplayFrame> frames_;
    int current_frame_{0};
    bool playing_{false};
    float playback_fps_{10.0f};
    float playback_timer_{0.0f};
    std::string loaded_file_;
    bool file_loaded_{false};
    char file_buf_[256]{"lidar_capture"};

    // For rendering the replay into its own FBO
    unsigned int fbo_{0};
    unsigned int tex_{0};
    int vp_width_{0};
    int vp_height_{0};
    void resizeFramebuffer(int w, int h);
};

} // namespace ols::ui
