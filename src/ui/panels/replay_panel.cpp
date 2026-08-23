#include <open_lidar_studio/ui/panels/replay_panel.hpp>
#include <imgui.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>

namespace ols::ui {

ReplayPanel::ReplayPanel() {
    std::memset(file_buf_, 0, sizeof(file_buf_));
    std::snprintf(file_buf_, sizeof(file_buf_), "%s", "lidar_capture");
}

void ReplayPanel::resizeFramebuffer(int w, int h) {
    if (w == vp_width_ && h == vp_height_) return;
    vp_width_ = w;
    vp_height_ = h;

    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    if (tex_) glDeleteTextures(1, &tex_);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool ReplayPanel::loadFile(const std::string& base) {
    std::ifstream meta(base + ".ols_meta", std::ios::binary);
    std::ifstream data(base + ".ols_data", std::ios::binary);
    if (!meta.is_open() || !data.is_open()) return false;

    frames_.clear();
    core::TelemetryHeader header;
    while (meta.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        std::vector<core::SerializedPoint> pts(header.point_count);
        if (!data.read(reinterpret_cast<char*>(pts.data()), header.point_count * sizeof(core::SerializedPoint)))
            break;
        frames_.push_back({header, std::move(pts)});
    }
    current_frame_ = 0;
    playing_ = false;
    return !frames_.empty();
}

void ReplayPanel::seekToFrame(int idx) {
    current_frame_ = std::max(0, std::min(idx, (int)frames_.size() - 1));
}

void ReplayPanel::render(UIState& state, rendering::GLRenderer& renderer) {
    ImGui::Begin("Replay");

    // ---- File loading ----
    ImGui::Text("Recording File");
    ImGui::SetNextItemWidth(-130);
    ImGui::InputText("##replay_file", file_buf_, sizeof(file_buf_));
    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(60, 0))) {
        loaded_file_ = file_buf_;
        file_loaded_ = loadFile(loaded_file_);
        if (!file_loaded_) {
            ImGui::OpenPopup("Load Error");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(55, 0))) {
        frames_.clear();
        file_loaded_ = false;
        playing_ = false;
    }

    if (ImGui::BeginPopupModal("Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Could not open '%s.ols_meta' or '%s.ols_data'", file_buf_, file_buf_);
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (!file_loaded_ || frames_.empty()) {
        ImGui::TextDisabled("No recording loaded. Use 'Logging & Playback' to record, then load here.");
        ImGui::End();
        return;
    }

    // ---- Frame info ----
    const ReplayFrame& rf = frames_[current_frame_];
    ImGui::Text("Frame %d / %d   |   Points: %u   |   R_inst: %.2fm",
        current_frame_ + 1, (int)frames_.size(),
        rf.header.point_count, rf.header.r_inst);

    // ---- Transport controls ----
    if (playing_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("  Pause  ")) playing_ = false;
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
        if (ImGui::Button("  Play   ")) playing_ = true;
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ImGui::Button("|<")) seekToFrame(0);
    ImGui::SameLine();
    if (ImGui::Button("<")) seekToFrame(current_frame_ - 1);
    ImGui::SameLine();
    if (ImGui::Button(">")) seekToFrame(current_frame_ + 1);
    ImGui::SameLine();
    if (ImGui::Button(">|")) seekToFrame((int)frames_.size() - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("FPS##replay", &playback_fps_, 1.0f, 60.0f);

    // Scrub bar
    int fi = current_frame_;
    if (ImGui::SliderInt("##scrub", &fi, 0, (int)frames_.size() - 1, "Frame %d")) {
        seekToFrame(fi);
        playing_ = false;
    }

    // Auto advance
    if (playing_) {
        playback_timer_ += ImGui::GetIO().DeltaTime;
        if (playback_timer_ >= 1.0f / playback_fps_) {
            playback_timer_ = 0.0f;
            if (current_frame_ < (int)frames_.size() - 1) {
                ++current_frame_;
            } else {
                playing_ = false; // end of recording
            }
        }
    }

    ImGui::Separator();

    // ---- Render current frame into FBO ----
    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = (int)avail.x;
    int h = (int)avail.y;
    if (w > 4 && h > 4) {
        resizeFramebuffer(w, h);
        renderer.resize(w, h);

        // Build render vertices (age=0 since it's a static frame)
        std::vector<rendering::RenderVertex> verts;
        verts.reserve(rf.points.size());
        for (const auto& pt : rf.points) {
            verts.push_back({pt.x, pt.y, pt.intensity, pt.range, 0.0f});
        }

        float r_display = rf.header.r_inst > 0.5f ? rf.header.r_inst * 1.1f : state.r_final;
        renderer.setMaxRange(r_display);
        renderer.setColorMode(state.color_mode);
        renderer.setHistoryDecay(0.0f); // no decay tint for replay
        renderer.updatePointCloud(verts);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, w, h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImVec2 img_pos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)(intptr_t)tex_, ImVec2{(float)w, (float)h}, ImVec2{0, 1}, ImVec2{1, 0});

        // Overlay rings
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 center = ImVec2(img_pos.x + w * 0.5f, img_pos.y + h * 0.5f);
        float pixels_per_meter = h / (2.0f * (r_display > 0.1f ? r_display : 0.1f));
        for (int i = 1; i <= 5; ++i) {
            float rm = r_display * (i / 5.0f);
            float rpx = rm * pixels_per_meter;
            dl->AddCircle(center, rpx, IM_COL32(100, 100, 100, 100), 64, 1.0f);
            char lbl[16]; snprintf(lbl, sizeof(lbl), "%.1fm", rm);
            dl->AddText(ImVec2(center.x + 4, center.y - rpx - 14), IM_COL32(180, 180, 180, 140), lbl);
        }
        dl->AddLine(ImVec2(center.x - 8, center.y), ImVec2(center.x + 8, center.y), IM_COL32(255,255,255,100));
        dl->AddLine(ImVec2(center.x, center.y - 8), ImVec2(center.x, center.y + 8), IM_COL32(255,255,255,100));
    }

    ImGui::End();
}

} // namespace ols::ui
