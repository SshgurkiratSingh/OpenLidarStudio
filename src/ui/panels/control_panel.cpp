#include <open_lidar_studio/ui/panels/control_panel.hpp>
#include <open_lidar_studio/hardware/serial_enumerator.hpp>
#include <imgui.h>
#include <cstdio>

namespace ols::ui {

ControlPanel::ControlPanel() {
}

void ControlPanel::refreshPorts(UIState& state) {
    ports_ = hardware::SerialEnumerator::getAvailablePorts();
    if (!ports_.empty() && state.selected_port.empty()) {
        state.selected_port = ports_[0];
    }
}

void ControlPanel::render(UIState& state, hardware::LidarController& controller, pipeline::SignalFilter& filter, pipeline::AutoRanger& ranger) {
    ImGui::Begin("Control & Toolbar");
    
    // Connection section
    if (ImGui::CollapsingHeader("Connection", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Refresh Ports")) {
            refreshPorts(state);
        }
        ImGui::SameLine();
        
        char port_buf[256];
        std::snprintf(port_buf, sizeof(port_buf), "%s", state.selected_port.c_str());
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##PortPath", port_buf, sizeof(port_buf))) {
            state.selected_port = port_buf;
        }
        ImGui::PopItemWidth();

        if (ImGui::BeginCombo("Detected Ports", "Select...")) {
            for (const auto& port : ports_) {
                if (ImGui::Selectable(port.c_str())) {
                    state.selected_port = port;
                }
            }
            ImGui::EndCombo();
        }
        
        const int baud_presets[] = {115200, 128000, 153600, 230400, 512000, 921600, 1000000};
        if (ImGui::BeginCombo("Baudrate", std::to_string(state.selected_baudrate).c_str())) {
            for (int baud : baud_presets) {
                bool is_selected = (state.selected_baudrate == baud);
                if (ImGui::Selectable(std::to_string(baud).c_str(), is_selected)) {
                    state.selected_baudrate = baud;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        ImGui::InputInt("Custom Baud", &state.custom_baudrate);
        ImGui::Checkbox("Single Channel (X2/S2)", &state.single_channel);
        
        if (!state.is_connected) {
            if (ImGui::Button("Connect")) {
                int baud = state.custom_baudrate > 0 ? state.custom_baudrate : state.selected_baudrate;
                if (controller.connect(state.selected_port, baud, state.single_channel)) {
                    state.is_connected = true;
                }
            }
        } else {
            if (ImGui::Button("Disconnect")) {
                controller.disconnect();
                state.is_connected = false;
                state.is_scanning = false;
            }
            ImGui::SameLine();
            
            if (!state.is_scanning) {
                if (ImGui::Button("Start Scan")) {
                    controller.startScan();
                    state.is_scanning = true;
                }
            } else {
                if (ImGui::Button("Stop Scan")) {
                    controller.stopScan();
                    state.is_scanning = false;
                }
            }
        }
    }
    
    // LiDAR Settings
    if (ImGui::CollapsingHeader("LiDAR Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Reset Defaults")) {
            state.target_rpm = 600.0f;
            state.auto_ranging = true;
            state.manual_range = 10.0f;
            state.alpha_min = 0.05f;
            state.alpha_max = 0.40f;
            state.k_sensitivity = 1.50f;
            state.min_angle_deg = 0.0f;
            state.max_angle_deg = 360.0f;
            state.min_radius = 0.1f;
            state.max_radius = 100.0f;
            state.color_mode = 0;
            state.point_size = 3.0f;
            state.circular_points = true;
            state.show_grid = true;
            state.grid_spacing_m = 0.5f;
            if (state.is_connected) {
                controller.setMotorFrequency(state.target_rpm / 60.0f);
            }
        }
        
        if (ImGui::SliderFloat("Target RPM", &state.target_rpm, 300.0f, 900.0f)) {
            controller.setMotorFrequency(state.target_rpm / 60.0f);
        }
    }
    
    // Visualization Panel options
    if (ImGui::CollapsingHeader("Visualization & Aesthetics")) {
        ImGui::Combo("Color Mode", &state.color_mode, "Range\0Intensity\0\0");
        ImGui::SliderFloat("Point Size", &state.point_size, 1.0f, 10.0f);
        ImGui::Checkbox("Circular Points", &state.circular_points);
        ImGui::Checkbox("Show Cartesian Grid", &state.show_grid);
        if (state.show_grid) {
            ImGui::SliderFloat("Grid Spacing (m)", &state.grid_spacing_m, 0.1f, 10.0f);
        }
        
        ImGui::Checkbox("Enable Motion History", &state.enable_history);
        if (state.enable_history) {
            ImGui::SliderInt("History Length", &state.history_length, 2, 30);
            ImGui::SliderFloat("Decay Rate", &state.history_decay, 0.1f, 5.0f);
        }
    }

    // Viewport Interactives & Tooling
    if (ImGui::CollapsingHeader("Interactive Tools")) {
        ImGui::Checkbox("Enable Measurement Mode", &state.measure_mode);
        if (state.measure_mode) {
            ImGui::TextColored(ImColor(0, 220, 255), "LMB: Drop point A / point B\nRMB: Reset measurement");
            if (state.measure_point_a_set) {
                ImGui::Text("A: (%.3f, %.3f) m", state.measure_ax, state.measure_ay);
            }
            if (state.measure_point_b_set) {
                ImGui::Text("B: (%.3f, %.3f) m", state.measure_bx, state.measure_by);
                float dx = state.measure_bx - state.measure_ax;
                float dy = state.measure_by - state.measure_ay;
                ImGui::Text("Distance: %.3f m", std::sqrt(dx*dx + dy*dy));
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Zoom & Pan Offset");
        ImGui::Text("Zoom: %.2fx", state.zoom);
        ImGui::Text("Pan: (%.2f, %.2f) m", state.pan_x, state.pan_y);
        if (ImGui::Button("Reset View")) {
            state.zoom = 1.0f;
            state.pan_x = 0.0f;
            state.pan_y = 0.0f;
        }

        ImGui::Separator();
        ImGui::Text("Viewport Capture");
        static char ss_name[128] = "screenshot";
        ImGui::InputText("Filename##SS", ss_name, sizeof(ss_name));
        if (ImGui::Button("Save Screenshot (PNG)")) {
            state.screenshot_filename = ss_name;
            state.request_screenshot = true;
        }
    }

    // Zone Alerts Configuration
    if (ImGui::CollapsingHeader("Zone Alerts")) {
        for (int i = 0; i < UIState::NUM_ZONES; ++i) {
            std::string label = "Zone " + std::to_string(i + 1);
            if (ImGui::TreeNode(label.c_str())) {
                auto& z = state.zones[i];
                ImGui::Checkbox("Enabled", &z.enabled);
                if (z.enabled) {
                    ImGui::Checkbox("Audio Alert", &z.play_sound);
                    ImGui::SliderFloat("Min Radius##z", &z.min_radius, 0.0f, 10.0f);
                    ImGui::SliderFloat("Max Radius##z", &z.max_radius, 0.1f, 20.0f);
                    ImGui::SliderFloat("Min Angle##z", &z.min_angle_deg, 0.0f, 360.0f);
                    ImGui::SliderFloat("Max Angle##z", &z.max_angle_deg, 0.0f, 360.0f);
                    if (z.triggered) {
                        ImGui::TextColored(ImColor(255, 100, 100), "STATUS: TRIGGERED!");
                    } else {
                        ImGui::TextColored(ImColor(100, 255, 100), "STATUS: Clear");
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    // Person Tracking
    if (ImGui::CollapsingHeader("Person Tracking")) {
        ImGui::Checkbox("Enable Person Tracking", &state.enable_person_tracking);
        if (state.enable_person_tracking) {
            ImGui::SliderFloat("Cluster Distance (m)", &state.track_cluster_dist, 0.05f, 1.0f);
            ImGui::SliderFloat("Min Size (m)", &state.track_min_size, 0.05f, 1.0f);
            ImGui::SliderFloat("Max Size (m)", &state.track_max_size, 0.2f, 2.0f);
            ImGui::SliderInt("Min Hits (Frames)", &state.track_min_hits, 1, 30);
            ImGui::Text("Tracking %zu objects", state.tracked_objects.size());
        }
    }
    
    // Auto-Ranger & Display
    if (ImGui::CollapsingHeader("Auto-Ranger & Display")) {
        if (ImGui::Checkbox("Auto-Ranging", &state.auto_ranging)) {
            ranger.setMode(state.auto_ranging);
        }
        
        if (!state.auto_ranging) {
            if (ImGui::SliderFloat("Manual Range", &state.manual_range, 1.0f, 100.0f)) {
                ranger.setManualRange(state.manual_range);
            }
        } else {
            if (ImGui::SliderFloat("Alpha Min", &state.alpha_min, 0.01f, 1.0f) ||
                ImGui::SliderFloat("Alpha Max", &state.alpha_max, 0.01f, 1.0f) ||
                ImGui::SliderFloat("k Sensitivity", &state.k_sensitivity, 0.1f, 5.0f)) {
                ranger.setParameters(state.alpha_min, state.alpha_max, state.k_sensitivity, 1.15f);
            }
        }
    }
    
    // Filters
    if (ImGui::CollapsingHeader("Filters (Culling)")) {
        if (ImGui::SliderFloat("Min Radius", &state.min_radius, 0.0f, 100.0f) ||
            ImGui::SliderFloat("Max Radius", &state.max_radius, 0.1f, 100.0f)) {
            filter.setRadiusBounds(state.min_radius, state.max_radius);
        }
        if (ImGui::SliderFloat("Min Angle", &state.min_angle_deg, 0.0f, 360.0f) ||
            ImGui::SliderFloat("Max Angle", &state.max_angle_deg, 0.0f, 360.0f)) {
            filter.setAngularCullRegion(state.min_angle_deg, state.max_angle_deg);
        }
    }

    // Help & Shortcuts section
    if (ImGui::CollapsingHeader("Help & Shortcuts")) {
        ImGui::Text("Viewport controls:");
        ImGui::BulletText("Scroll Wheel: Zoom");
        ImGui::BulletText("Middle Mouse Button Drag: Pan");
        ImGui::BulletText("Double Click MMB: Reset Zoom/Pan");
        ImGui::BulletText("LMB (Measurement Mode): Set Points A/B");
        ImGui::BulletText("RMB (Measurement Mode): Clear Points");
        ImGui::BulletText("Hover Point: Distance & Angle tooltip");
    }
    
    ImGui::End();
}

} // namespace ols::ui
