#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>
#include <open_lidar_studio/rendering/gl_renderer.hpp>
#include <memory>
#include <string>
#include <imgui.h>

namespace ols::ui {

class ViewportPanel {
public:
    ViewportPanel();
    ~ViewportPanel() = default;

    void render(UIState& state, rendering::GLRenderer& renderer);

private:
    void drawOverlay(UIState& state, const ImVec2& pos, const ImVec2& size,
                     float pixels_per_meter, float effective_range);
    void drawGrid(ImDrawList* dl, const ImVec2& center, const ImVec2& size,
                  float pixels_per_meter, float effective_range, float spacing_m);
    void drawZones(ImDrawList* dl, const ImVec2& center,
                   float pixels_per_meter, UIState& state, float dt);
    void drawMeasureTool(ImDrawList* dl, const ImVec2& center,
                         float pixels_per_meter, UIState& state, bool hovered);
    void drawTrackedObjects(ImDrawList* dl, const ImVec2& center,
                            float pixels_per_meter, const UIState& state);
    void handleZoomPan(UIState& state, const ImVec2& size,
                       float pixels_per_meter, float effective_range);
    void takeScreenshot(rendering::GLRenderer& renderer, const std::string& filename);

    unsigned int fbo_{0};
    unsigned int tex_{0};
    int width_{0};
    int height_{0};

    void resizeFramebuffer(int w, int h);
};

} // namespace ols::ui
