#include <open_lidar_studio/ui/panels/logging_panel.hpp>
#include <imgui.h>
#include <vector>

namespace ols::ui {

LoggingPanel::LoggingPanel() {
}

void LoggingPanel::render(UIState& state, pipeline::AsyncLogger& logger) {
    ImGui::Begin("Logging & Playback");

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", state.log_filename.c_str());
    if (ImGui::InputText("Filename", buf, sizeof(buf))) {
        state.log_filename = buf;
    }

    if (!state.is_logging) {
        if (ImGui::Button("Start Recording", ImVec2(120, 30))) {
            logger.startLogging(state.log_filename);
            state.is_logging = true;
            state.log_duration = 0.0f;
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Stop Recording", ImVec2(120, 30))) {
            logger.stopLogging();
            state.is_logging = false;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Duration: %.1fs", state.log_duration);
    }
    
    ImGui::Separator();
    ImGui::Text("Data Export");
    if (ImGui::Button("Export to CSV")) {
        pipeline::AsyncLogger::exportToCSV(state.log_filename + ".ols_meta", state.log_filename + ".ols_data", state.log_filename + ".csv");
    }
    ImGui::SameLine();
    if (ImGui::Button("Export to JSON")) {
        pipeline::AsyncLogger::exportToJSON(state.log_filename + ".ols_meta", state.log_filename + ".ols_data", state.log_filename + ".json");
    }
    
    ImGui::End();
}

} // namespace ols::ui
