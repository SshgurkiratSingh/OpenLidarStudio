#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/rendering/gl_renderer.hpp>
#include <memory>
#include <imgui.h>

namespace ols::ui {

class ViewportPanel {
public:
    ViewportPanel();
    ~ViewportPanel() = default;

    void render(UIState& state, rendering::GLRenderer& renderer);

private:
    void drawOverlay(const UIState& state, const ImVec2& pos, const ImVec2& size);
    
    // Custom framebuffer for ImGui texture binding
    unsigned int fbo_{0};
    unsigned int tex_{0};
    int width_{0};
    int height_{0};
    
    void resizeFramebuffer(int w, int h);
};

} // namespace ols::ui
