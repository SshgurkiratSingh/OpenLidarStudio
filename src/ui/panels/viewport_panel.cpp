#include <open_lidar_studio/ui/panels/viewport_panel.hpp>
#include <imgui.h>
#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

// stb_image_write — header-only PNG writer (single-header, no extra dep)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <open_lidar_studio/third_party/stb_image_write.h>

namespace ols::ui {

static constexpr float PI = 3.14159265f;

ViewportPanel::ViewportPanel() {}

// ─── Framebuffer ────────────────────────────────────────────────────────────

void ViewportPanel::resizeFramebuffer(int w, int h) {
    if (w == width_ && h == height_) return;
    width_ = w; height_ = h;

    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    if (tex_) glDeleteTextures(1, &tex_);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer incomplete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─── Screenshot ──────────────────────────────────────────────────────────────

void ViewportPanel::takeScreenshot(rendering::GLRenderer& renderer, const std::string& filename) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    std::vector<unsigned char> pixels;
    renderer.readPixels(pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int w = renderer.getWidth();
    int h = renderer.getHeight();

    // Flip vertically (OpenGL origin is bottom-left)
    std::vector<unsigned char> flipped(pixels.size());
    for (int row = 0; row < h; ++row) {
        std::memcpy(flipped.data() + row * w * 4,
                    pixels.data() + (h - 1 - row) * w * 4,
                    w * 4);
    }

    std::string path = filename + ".png";
    stbi_write_png(path.c_str(), w, h, 4, flipped.data(), w * 4);
    std::cout << "[Screenshot] Saved: " << path << "\n";
}

// ─── Main render ─────────────────────────────────────────────────────────────

void ViewportPanel::render(UIState& state, rendering::GLRenderer& renderer) {
    ImGui::Begin("Viewport", nullptr,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = static_cast<int>(avail.x);
    int h = static_cast<int>(avail.y);

    if (w > 0 && h > 0) {
        resizeFramebuffer(w, h);

        // Push new settings to renderer
        renderer.resize(w, h);
        renderer.setMaxRange(state.r_final);
        renderer.setColorMode(state.color_mode);
        renderer.setHistoryDecay(state.history_decay);
        renderer.setPointSize(state.point_size);
        renderer.setCircularPoints(state.circular_points);
        renderer.setZoomPan(state.zoom, state.pan_x, state.pan_y);

        // Render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, w, h);
        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Screenshot (before unbinding ensures we capture the right FBO)
        if (state.request_screenshot) {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
            takeScreenshot(renderer, state.screenshot_filename);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            state.request_screenshot = false;
        }

        // Blit texture to ImGui
        ImVec2 img_pos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)(intptr_t)tex_,
                     ImVec2{static_cast<float>(w), static_cast<float>(h)},
                     ImVec2{0, 1}, ImVec2{1, 0});

        // Derived display params
        float r_display = state.r_final > 0.1f ? state.r_final : 0.1f;
        float effective_range = r_display / state.zoom;
        float pixels_per_meter = (static_cast<float>(h) * state.zoom) / (2.0f * r_display);

        drawOverlay(state, img_pos, avail, pixels_per_meter, effective_range);
        handleZoomPan(state, avail, pixels_per_meter, effective_range);
    }

    ImGui::End();
}

// ─── Zoom & Pan ──────────────────────────────────────────────────────────────

void ViewportPanel::handleZoomPan(UIState& state, const ImVec2& size,
                                  float /*ppm*/, float /*range*/) {
    if (!ImGui::IsItemHovered()) return;
    ImGuiIO& io = ImGui::GetIO();

    // Scroll wheel → zoom
    if (io.MouseWheel != 0.0f) {
        state.zoom *= (1.0f + io.MouseWheel * 0.1f);
        state.zoom = std::max(0.2f, std::min(state.zoom, 20.0f));
    }

    // Middle-mouse drag → pan (in world-space metres)
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        float r_display = state.r_final > 0.1f ? state.r_final : 0.1f;
        float ppm = (size.y * state.zoom) / (2.0f * r_display);
        state.pan_x -= delta.x / ppm;
        state.pan_y += delta.y / ppm;
    }

    // Double-click middle → reset zoom/pan
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle)) {
        state.zoom  = 1.0f;
        state.pan_x = 0.0f;
        state.pan_y = 0.0f;
    }
}

// ─── Grid ────────────────────────────────────────────────────────────────────

void ViewportPanel::drawGrid(ImDrawList* dl, const ImVec2& center, const ImVec2& size,
                             float ppm, float range, float spacing_m) {
    if (spacing_m <= 0.0f) return;
    ImU32 col = IM_COL32(60, 60, 75, 90);

    // Vertical lines
    float x_start = center.x - std::fmod(center.x - (size.x * 0.0f), spacing_m * ppm);
    for (float x = center.x - range * ppm; x <= center.x + range * ppm; x += spacing_m * ppm) {
        dl->AddLine(ImVec2(x, center.y - range * ppm),
                    ImVec2(x, center.y + range * ppm), col, 0.5f);
    }
    // Horizontal lines
    for (float y = center.y - range * ppm; y <= center.y + range * ppm; y += spacing_m * ppm) {
        dl->AddLine(ImVec2(center.x - range * ppm, y),
                    ImVec2(center.x + range * ppm, y), col, 0.5f);
    }
    (void)x_start;
}

// ─── Zone alerts ─────────────────────────────────────────────────────────────

void ViewportPanel::drawZones(ImDrawList* dl, const ImVec2& center,
                              float ppm, UIState& state, float dt) {
    for (int zi = 0; zi < UIState::NUM_ZONES; ++zi) {
        auto& z = state.zones[zi];
        if (!z.enabled) continue;

        // Flash timer
        if (z.triggered) z.trigger_flash = 0.4f;
        if (z.trigger_flash > 0.0f) z.trigger_flash -= dt;

        bool flashing = z.trigger_flash > 0.0f;
        ImU32 ring_col = flashing
            ? IM_COL32(255, 60, 60, 180)
            : IM_COL32(255, 180, 60, 100);

        float r_min_px = z.min_radius * ppm;
        float r_max_px = z.max_radius * ppm;

        // Draw arc segments approximating the angular sector
        int   segs = 48;
        float ang0  = z.min_angle_deg * PI / 180.0f;
        float ang1  = z.max_angle_deg * PI / 180.0f;

        // Draw two arcs + two radial edges
        for (int s = 0; s < segs; ++s) {
            float a0 = ang0 + (ang1 - ang0) * (s    ) / segs;
            float a1 = ang0 + (ang1 - ang0) * (s + 1) / segs;
            // Outer arc
            dl->AddLine(
                ImVec2(center.x + r_max_px * std::cos(a0), center.y - r_max_px * std::sin(a0)),
                ImVec2(center.x + r_max_px * std::cos(a1), center.y - r_max_px * std::sin(a1)),
                ring_col, flashing ? 2.5f : 1.5f);
            // Inner arc
            dl->AddLine(
                ImVec2(center.x + r_min_px * std::cos(a0), center.y - r_min_px * std::sin(a0)),
                ImVec2(center.x + r_min_px * std::cos(a1), center.y - r_min_px * std::sin(a1)),
                ring_col, flashing ? 2.5f : 1.5f);
        }
        // Radial edges
        dl->AddLine(ImVec2(center.x + r_min_px * std::cos(ang0), center.y - r_min_px * std::sin(ang0)),
                    ImVec2(center.x + r_max_px * std::cos(ang0), center.y - r_max_px * std::sin(ang0)),
                    ring_col, 1.5f);
        dl->AddLine(ImVec2(center.x + r_min_px * std::cos(ang1), center.y - r_min_px * std::sin(ang1)),
                    ImVec2(center.x + r_max_px * std::cos(ang1), center.y - r_max_px * std::sin(ang1)),
                    ring_col, 1.5f);

        // Label
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "Z%d%s", zi + 1, flashing ? " !" : "");
        float mid_ang = (ang0 + ang1) * 0.5f;
        float mid_r   = (r_min_px + r_max_px) * 0.5f;
        dl->AddText(ImVec2(center.x + mid_r * std::cos(mid_ang) - 8,
                           center.y - mid_r * std::sin(mid_ang) - 7),
                    ring_col, lbl);
    }
}

// ─── Measurement tool ────────────────────────────────────────────────────────

void ViewportPanel::drawMeasureTool(ImDrawList* dl, const ImVec2& center,
                                    float ppm, UIState& state, bool hovered) {
    if (!state.measure_mode) return;

    ImU32 col_pt  = IM_COL32(0, 220, 255, 220);
    ImU32 col_ln  = IM_COL32(0, 220, 255, 160);

    auto world_to_screen = [&](float wx, float wy) -> ImVec2 {
        return ImVec2(center.x + wx * ppm, center.y - wy * ppm);
    };
    auto screen_to_world = [&](ImVec2 sp) -> std::pair<float,float> {
        return {(sp.x - center.x) / ppm, (center.y - sp.y) / ppm};
    };

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mp = ImGui::GetMousePos();
        auto [wx, wy] = screen_to_world(mp);
        if (!state.measure_point_a_set) {
            state.measure_ax = wx; state.measure_ay = wy;
            state.measure_point_a_set = true;
            state.measure_point_b_set = false;
        } else {
            state.measure_bx = wx; state.measure_by = wy;
            state.measure_point_b_set = true;
        }
    }

    // Draw point A
    if (state.measure_point_a_set) {
        ImVec2 sa = world_to_screen(state.measure_ax, state.measure_ay);
        dl->AddCircleFilled(sa, 5.0f, col_pt);
        dl->AddText(ImVec2(sa.x + 7, sa.y - 14), col_pt, "A");
    }

    // Draw live line from A to mouse / to B
    if (state.measure_point_a_set) {
        ImVec2 sa = world_to_screen(state.measure_ax, state.measure_ay);
        ImVec2 end_pt;
        float dist_m = 0.0f;

        if (state.measure_point_b_set) {
            end_pt = world_to_screen(state.measure_bx, state.measure_by);
            float dx = state.measure_bx - state.measure_ax;
            float dy = state.measure_by - state.measure_ay;
            dist_m = std::sqrt(dx*dx + dy*dy);
        } else if (hovered) {
            end_pt = ImGui::GetMousePos();
            auto [wx, wy] = screen_to_world(end_pt);
            float dx = wx - state.measure_ax;
            float dy = wy - state.measure_ay;
            dist_m = std::sqrt(dx*dx + dy*dy);
        } else {
            end_pt = sa;
        }

        if (state.measure_point_b_set || hovered) {
            dl->AddLine(sa, end_pt, col_ln, 1.5f);
            // Mid-point label
            ImVec2 mid = ImVec2((sa.x + end_pt.x) * 0.5f, (sa.y + end_pt.y) * 0.5f);
            char dist_lbl[32];
            std::snprintf(dist_lbl, sizeof(dist_lbl), "%.3f m", dist_m);
            dl->AddRectFilled(ImVec2(mid.x - 2, mid.y - 10),
                              ImVec2(mid.x + 60, mid.y + 4),
                              IM_COL32(10, 10, 20, 180), 3.0f);
            dl->AddText(ImVec2(mid.x, mid.y - 9), col_pt, dist_lbl);
        }

        if (state.measure_point_b_set) {
            ImVec2 sb = world_to_screen(state.measure_bx, state.measure_by);
            dl->AddCircleFilled(sb, 5.0f, col_pt);
            dl->AddText(ImVec2(sb.x + 7, sb.y - 14), col_pt, "B");
        }
    }

    // Right-click clears measurement
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        state.measure_point_a_set = false;
        state.measure_point_b_set = false;
    }
}

void ViewportPanel::drawLedWallTool(ImDrawList* dl, const ImVec2& center,
                                    float ppm, UIState& state, bool hovered) {
    if (!state.led_wall.enabled) return;

    ImU32 col_pt  = IM_COL32(255, 0, 200, 220); // Pinkish for LEDs
    ImU32 col_ln  = IM_COL32(255, 0, 200, 160);

    auto world_to_screen = [&](float wx, float wy) -> ImVec2 {
        return ImVec2(center.x + wx * ppm, center.y - wy * ppm);
    };
    auto screen_to_world = [&](ImVec2 sp) -> std::pair<float,float> {
        return {(sp.x - center.x) / ppm, (center.y - sp.y) / ppm};
    };

    if (state.led_wall.define_mode && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mp = ImGui::GetMousePos();
        auto [wx, wy] = screen_to_world(mp);
        if (!state.led_wall.define_point_a_set) {
            state.led_wall.ax = wx; state.led_wall.ay = wy;
            state.led_wall.define_point_a_set = true;
            state.led_wall.define_point_b_set = false;
        } else {
            state.led_wall.bx = wx; state.led_wall.by = wy;
            state.led_wall.define_point_b_set = true;
            state.led_wall.define_mode = false; // Auto exit mode
        }
    }

    if (state.led_wall.define_point_a_set) {
        ImVec2 sa = world_to_screen(state.led_wall.ax, state.led_wall.ay);
        dl->AddCircleFilled(sa, 5.0f, col_pt);
        dl->AddText(ImVec2(sa.x + 7, sa.y - 14), col_pt, "LED Wall 0");
    }

    if (state.led_wall.define_point_a_set) {
        ImVec2 sa = world_to_screen(state.led_wall.ax, state.led_wall.ay);
        ImVec2 end_pt;
        if (state.led_wall.define_point_b_set) {
            end_pt = world_to_screen(state.led_wall.bx, state.led_wall.by);
        } else if (state.led_wall.define_mode && hovered) {
            end_pt = ImGui::GetMousePos();
        } else {
            end_pt = sa;
        }

        if (state.led_wall.define_point_b_set || (state.led_wall.define_mode && hovered)) {
            // Draw a thicker line to represent the LED strip
            dl->AddLine(sa, end_pt, col_ln, 4.0f);
        }

        if (state.led_wall.define_point_b_set) {
            ImVec2 sb = world_to_screen(state.led_wall.bx, state.led_wall.by);
            dl->AddCircleFilled(sb, 5.0f, col_pt);
            dl->AddText(ImVec2(sb.x + 7, sb.y - 14), col_pt, "LED Wall N");
        }
    }

    if (state.led_wall.define_mode && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        state.led_wall.define_point_a_set = false;
        state.led_wall.define_point_b_set = false;
    }
}

// ─── Overlay (rings, grid, crosshair, hover, zones, measure) ─────────────────

void ViewportPanel::drawOverlay(UIState& state, const ImVec2& pos, const ImVec2& size,
                                float ppm, float effective_range) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Center accounts for pan: pan moves the world, not the screen center
    ImVec2 center = ImVec2(pos.x + size.x * 0.5f - state.pan_x * ppm,
                           pos.y + size.y * 0.5f + state.pan_y * ppm);

    float dt = ImGui::GetIO().DeltaTime;

    // Cartesian grid
    if (state.show_grid) {
        drawGrid(dl, center, size, ppm, effective_range, state.grid_spacing_m);
    }

    // Concentric distance rings
    float r = effective_range;
    for (int i = 1; i <= 5; ++i) {
        float ring_m  = r * (i / 5.0f);
        float ring_px = ring_m * ppm;
        ImU32 ring_col = (i == 5) ? IM_COL32(140, 140, 160, 160)
                                  : IM_COL32(80, 80, 100, 100);
        dl->AddCircle(center, ring_px, ring_col, 96, 1.0f);

        char lbl[24];
        std::snprintf(lbl, sizeof(lbl), "%.1fm", ring_m);
        dl->AddText(ImVec2(center.x + 4, center.y - ring_px - 14),
                    IM_COL32(180, 180, 200, 140), lbl);
    }

    // Cardinal direction markers
    for (int deg = 0; deg < 360; deg += 45) {
        float rad = deg * PI / 180.0f;
        float rpx = r * ppm;
        ImVec2 tip = ImVec2(center.x + rpx * std::cos(rad), center.y - rpx * std::sin(rad));
        const char* labels[] = {"E","NE","N","NW","W","SW","S","SE"};
        dl->AddText(ImVec2(tip.x - 4, tip.y - 8),
                    IM_COL32(120, 140, 160, 130), labels[deg/45]);
    }

    // Crosshair
    dl->AddLine(ImVec2(center.x - 12, center.y), ImVec2(center.x + 12, center.y),
                IM_COL32(255, 255, 255, 100));
    dl->AddLine(ImVec2(center.x, center.y - 12), ImVec2(center.x, center.y + 12),
                IM_COL32(255, 255, 255, 100));

    // Zone alerts
    drawZones(dl, center, ppm, state, dt);

    // Tracked Objects
    drawTrackedObjects(dl, center, ppm, state);

    // Measure tool
    bool hovered = ImGui::IsItemHovered();
    drawMeasureTool(dl, center, ppm, state, hovered);
    drawLedWallTool(dl, center, ppm, state, hovered);

    // Hover tooltip (only when not in measure mode)
    if (!state.measure_mode && hovered) {
        ImVec2 mp = ImGui::GetMousePos();
        float dx = mp.x - center.x;
        float dy = center.y - mp.y;
        float dist_m = std::sqrt(dx*dx + dy*dy) / ppm;

        if (dist_m <= r * 1.1f) {
            float angle_rad = std::atan2(dy, dx);
            float angle_deg = angle_rad * 180.0f / PI;
            if (angle_deg < 0) angle_deg += 360.0f;

            ImGui::BeginTooltip();
            ImGui::Text("Distance: %.3f m", dist_m);
            ImGui::Text("Angle:    %.1f°", angle_deg);
            ImGui::Text("[Scroll] Zoom  [Mid-drag] Pan");
            ImGui::EndTooltip();

            dl->AddLine(center, mp, IM_COL32(255, 200, 0, 120), 1.0f);
        }
    }

    // Zoom level indicator (top-right corner)
    char zoom_lbl[32];
    std::snprintf(zoom_lbl, sizeof(zoom_lbl), "%.1fx", state.zoom);
    ImVec2 zoom_pos = ImVec2(pos.x + size.x - 55, pos.y + 8);
    dl->AddRectFilled(ImVec2(zoom_pos.x - 4, zoom_pos.y - 2),
                      ImVec2(zoom_pos.x + 45, zoom_pos.y + 14),
                      IM_COL32(10, 10, 20, 160), 3.0f);
    dl->AddText(zoom_pos, IM_COL32(180, 220, 255, 200), zoom_lbl);

    // Measure mode indicator
    if (state.measure_mode) {
        const char* hint = state.measure_point_a_set ? (state.measure_point_b_set
            ? "Click A to restart | RMB clear" : "Click B")
            : "Click A";
        dl->AddRectFilled(ImVec2(pos.x + 6, pos.y + 6),
                          ImVec2(pos.x + 240, pos.y + 22),
                          IM_COL32(0, 40, 60, 200), 4.0f);
        dl->AddText(ImVec2(pos.x + 10, pos.y + 8),
                    IM_COL32(0, 220, 255, 240), "MEASURE MODE —");
        dl->AddText(ImVec2(pos.x + 130, pos.y + 8),
                    IM_COL32(180, 240, 255, 200), hint);
    } else if (state.led_wall.define_mode) {
        const char* hint = state.led_wall.define_point_a_set ? "Click B to finish" : "Click A to start";
        dl->AddRectFilled(ImVec2(pos.x + 6, pos.y + 6),
                          ImVec2(pos.x + 300, pos.y + 22),
                          IM_COL32(40, 0, 40, 200), 4.0f);
        dl->AddText(ImVec2(pos.x + 10, pos.y + 8),
                    IM_COL32(255, 50, 200, 240), "DEFINE WALL MODE —");
        dl->AddText(ImVec2(pos.x + 150, pos.y + 8),
                    IM_COL32(255, 150, 200, 200), hint);
    }
}

// ─── Person Tracking Overlay ──────────────────────────────────────────────────

void ViewportPanel::drawTrackedObjects(ImDrawList* dl, const ImVec2& center,
                                       float ppm, const UIState& state) {
    if (!state.enable_person_tracking) return;

    for (const auto& obj : state.tracked_objects) {
        ImVec2 screen_pos(center.x + obj.center_x * ppm, center.y - obj.center_y * ppm);
        float radius_px = obj.radius * ppm;
        
        // Draw bounding box/circle
        ImU32 color = IM_COL32(50, 255, 50, 200);
        if (obj.missing_frames > 0) {
            color = IM_COL32(200, 200, 50, 150); // Yellowish and more transparent if missing
        }
        dl->AddCircle(screen_pos, radius_px, color, 32, 2.0f);
        
        // Velocity vector
        if (std::abs(obj.velocity_x) > 0.01f || std::abs(obj.velocity_y) > 0.01f) {
            ImVec2 vel_end(screen_pos.x + (obj.center_x + obj.velocity_x) * ppm, screen_pos.y - (obj.center_y + obj.velocity_y) * ppm);
            dl->AddLine(screen_pos, vel_end, IM_COL32(50, 255, 50, 150), 2.0f);
        }
        
        // Label
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "ID %d", obj.id);
        dl->AddText(ImVec2(screen_pos.x + radius_px + 5, screen_pos.y - radius_px - 5), color, lbl);
    }
}

} // namespace ols::ui
