#include <open_lidar_studio/ui/panels/telemetry_panel.hpp>
#include <imgui.h>
#include <implot.h>

namespace ols::ui {

TelemetryPanel::TelemetryPanel() {
}

void TelemetryPanel::render(UIState& state) {
    ImGui::Begin("Telemetry & Analytics");

    // Stats row
    ImGui::Text("Points / Sweep:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.f), "%zu", state.point_count);
    ImGui::SameLine(0, 20);
    ImGui::Text("RPM:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 0.9f, 1.f), "%.1f", state.current_rpm);
    ImGui::SameLine(0, 20);
    ImGui::Text("Range:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.f), "%.2fm", state.r_final);

    ImGui::Separator();

    // Scrolling 30-second window
    float t_now = state.current_time;
    float t_min = t_now - 30.0f;
    float r_max = state.r_final > 1.0f ? state.r_final * 1.1f : 5.0f;

    if (ImPlot::BeginPlot("Range Dynamics", ImVec2(-1, 180))) {
        ImPlot::SetupAxes("Time (s)", "Range (m)");
        ImPlot::SetupAxisLimits(ImAxis_X1, t_min, t_now, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, r_max, ImPlotCond_Always);
        if (!state.time_history.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.25f, 0.65f, 1.0f, 1.f));
            ImPlot::PlotLine("Inst Range", state.time_history.data(), state.r_inst_history.data(), (int)state.time_history.size());
            ImPlot::PopStyleColor();
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.5f, 0.1f, 1.f));
            ImPlot::PlotLine("Smooth Range", state.time_history.data(), state.r_smooth_history.data(), (int)state.time_history.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Alpha Adaptive", ImVec2(-1, 140))) {
        ImPlot::SetupAxes("Time (s)", "Alpha");
        ImPlot::SetupAxisLimits(ImAxis_X1, t_min, t_now, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0, ImPlotCond_Always);
        if (!state.time_history.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.6f, 1.0f, 0.4f, 1.f));
            ImPlot::PlotLine("Alpha", state.time_history.data(), state.alpha_history.data(), (int)state.time_history.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Point Count", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Time (s)", "Points");
        ImPlot::SetupAxisLimits(ImAxis_X1, t_min, t_now, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 600.0, ImPlotCond_Always);
        if (!state.time_history.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.3f, 0.8f, 1.f));
            ImPlot::PlotLine("Points", state.time_history.data(), state.point_history.data(), (int)state.time_history.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }

    ImGui::End();
}

} // namespace ols::ui
