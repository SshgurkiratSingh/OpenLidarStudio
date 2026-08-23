#include <open_lidar_studio/ui/panels/viewport_panel.hpp>
#include <imgui.h>
#include <glad/glad.h>
#include <iostream>
#include <cmath>

namespace ols::ui {

ViewportPanel::ViewportPanel() {
}

void ViewportPanel::resizeFramebuffer(int w, int h) {
    if (w == width_ && h == height_) return;
    
    width_ = w;
    height_ = h;

    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    if (tex_) glDeleteTextures(1, &tex_);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ViewportPanel::render(UIState& state, rendering::GLRenderer& renderer) {
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    int w = static_cast<int>(viewportPanelSize.x);
    int h = static_cast<int>(viewportPanelSize.y);

    if (w > 0 && h > 0) {
        resizeFramebuffer(w, h);
        renderer.resize(w, h);
        
        // Render scene to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, w, h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderer.render();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Display texture in ImGui
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)(intptr_t)tex_, ImVec2{static_cast<float>(w), static_cast<float>(h)}, ImVec2{0, 1}, ImVec2{1, 0});
        
        drawOverlay(state, pos, viewportPanelSize);
    }
    
    ImGui::End();
}

void ViewportPanel::drawOverlay(const UIState& state, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    
    // Draw crosshair
    draw_list->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y), IM_COL32(255, 255, 255, 128));
    draw_list->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10), IM_COL32(255, 255, 255, 128));
    
    // Draw concentric rings based on r_final
    float r = state.r_final > 0.1f ? state.r_final : 0.1f;
    // Map radius in meters to pixels: height 2*r maps to size.y
    float pixels_per_meter = size.y / (2.0f * r);
    
    for (int i = 1; i <= 5; ++i) {
        float ring_radius_m = r * (i / 5.0f);
        float ring_radius_px = ring_radius_m * pixels_per_meter;
        draw_list->AddCircle(center, ring_radius_px, IM_COL32(100, 100, 100, 100), 64, 1.0f);
        
        char label[32];
        snprintf(label, sizeof(label), "%.1fm", ring_radius_m);
        draw_list->AddText(ImVec2(center.x + 5, center.y - ring_radius_px - 15), IM_COL32(200, 200, 200, 150), label);
    }
    
    // Hover tooltip
    if (ImGui::IsItemHovered()) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float dx = mouse_pos.x - center.x;
        float dy = center.y - mouse_pos.y; // Y is flipped in ImGui vs math
        float dist_m = std::sqrt(dx*dx + dy*dy) / pixels_per_meter;
        
        // Only show tooltip if within range
        if (dist_m <= r) {
            float angle_rad = std::atan2(dy, dx);
            float angle_deg = angle_rad * 180.0f / 3.14159265f;
            if (angle_deg < 0) angle_deg += 360.0f;
            
            ImGui::BeginTooltip();
            ImGui::Text("Hover Distance: %.2f m", dist_m);
            ImGui::Text("Hover Angle: %.1f°", angle_deg);
            ImGui::EndTooltip();
            
            // Draw line from center to mouse
            draw_list->AddLine(center, mouse_pos, IM_COL32(255, 200, 0, 150), 1.0f);
        }
    }
}

} // namespace ols::ui
