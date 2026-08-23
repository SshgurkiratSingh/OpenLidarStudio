#include <open_lidar_studio/ui/panels/control_panel.hpp>
#include <open_lidar_studio/hardware/serial_enumerator.hpp>
#include <imgui.h>

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
    
    if (ImGui::Button("Refresh Ports")) {
        refreshPorts(state);
    }
    ImGui::SameLine();
    
    char port_buf[256];
    snprintf(port_buf, sizeof(port_buf), "%s", state.selected_port.c_str());
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
    
    ImGui::Separator();
    ImGui::Text("LiDAR Parameters");
    ImGui::SameLine(ImGui::GetWindowWidth() - 120);
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
        if (state.is_connected) {
            controller.setMotorFrequency(state.target_rpm / 60.0f);
        }
    }
    
    if (ImGui::SliderFloat("Target RPM", &state.target_rpm, 300.0f, 900.0f)) { // 5-15 Hz is 300-900 RPM
        controller.setMotorFrequency(state.target_rpm / 60.0f);
    }
    
    ImGui::Separator();
    ImGui::Text("Visualization & History");
    ImGui::Combo("Color Mode", &state.color_mode, "Range\0Intensity\0\0");
    ImGui::Checkbox("Enable Motion History", &state.enable_history);
    if (state.enable_history) {
        ImGui::SliderInt("History Length", &state.history_length, 2, 30);
        ImGui::SliderFloat("Decay Rate", &state.history_decay, 0.1f, 5.0f);
    }
    
    ImGui::Separator();
    ImGui::Text("Auto-Ranger & Display");
    
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
    
    ImGui::Separator();
    ImGui::Text("Filters (Culling)");
    if (ImGui::SliderFloat("Min Radius", &state.min_radius, 0.0f, 100.0f) ||
        ImGui::SliderFloat("Max Radius", &state.max_radius, 0.1f, 100.0f)) {
        filter.setRadiusBounds(state.min_radius, state.max_radius);
    }
    if (ImGui::SliderFloat("Min Angle", &state.min_angle_deg, 0.0f, 360.0f) ||
        ImGui::SliderFloat("Max Angle", &state.max_angle_deg, 0.0f, 360.0f)) {
        filter.setAngularCullRegion(state.min_angle_deg, state.max_angle_deg);
    }
    
    ImGui::End();
}

} // namespace ols::ui
