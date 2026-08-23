#include <open_lidar_studio/ui/app_window.hpp>
#include <open_lidar_studio/core/math_utils.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <implot.h>
#include <chrono>
#include <iostream>

namespace ols::ui {

AppWindow::AppWindow(int width, int height, const char* title)
    : width_(width), height_(height), title_(title) {
}

AppWindow::~AppWindow() {
    lidar_controller_.disconnect();
    
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool AppWindow::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width_, height_, title_, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    if (!gl_renderer_.initialize()) {
        std::cerr << "Failed to initialize GLRenderer" << std::endl;
        return false;
    }

    // Connect Lidar Callback
    lidar_controller_.setScanCallback([this](const LaserScan& scan) {
        std::lock_guard<std::mutex> lock(scan_mutex_);
        latest_scan_ = scan.points;
        new_scan_available_ = true;
    });

    return true;
}

void AppWindow::setupDockspace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        // Split: LEFT (25%) | CENTER | RIGHT (22%)
        ImGuiID dock_center = dockspace_id;
        ImGuiID dock_left   = ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Left,  0.25f, nullptr, &dock_center);
        ImGuiID dock_right  = ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Right, 0.28f, nullptr, &dock_center);

        // Left sidebar: Control on top, Logging on bottom
        ImGuiID dock_left_bottom;
        ImGuiID dock_left_top = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.70f, nullptr, &dock_left_bottom);

        // Right sidebar: Telemetry
        // Center: Viewport tab + Replay tab
        ImGui::DockBuilderDockWindow("Control & Toolbar",    dock_left_top);
        ImGui::DockBuilderDockWindow("Logging & Playback",   dock_left_bottom);
        ImGui::DockBuilderDockWindow("Telemetry & Analytics",dock_right);
        ImGui::DockBuilderDockWindow("Viewport",             dock_center);
        ImGui::DockBuilderDockWindow("Replay",               dock_center); // tabs with Viewport
        
        ImGui::DockBuilderFinish(dockspace_id);
    }
    
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    
    ImGui::End();
}

void AppWindow::processScanData() {
    std::vector<LaserPoint> current_points;
    {
        std::lock_guard<std::mutex> lock(scan_mutex_);
        if (new_scan_available_) {
            current_points = latest_scan_;
            new_scan_available_ = false;
        }
    }

    if (!current_points.empty()) {
        std::vector<LaserPoint> filtered_points;
        signal_filter_.process(current_points, filtered_points);
        
        auto_ranger_.process(filtered_points);

        state_.point_count = filtered_points.size();
        state_.r_inst = auto_ranger_.getInstRange();
        state_.r_smooth = auto_ranger_.getSmoothRange();
        state_.alpha_adaptive = auto_ranger_.getAlphaAdaptive();
        state_.r_final = auto_ranger_.getFinalRange();
        
        gl_renderer_.setMaxRange(state_.r_final);
        gl_renderer_.setColorMode(state_.color_mode);
        gl_renderer_.setHistoryDecay(state_.history_decay);

        std::vector<core::SerializedPoint> serialized_points;
        serialized_points.reserve(filtered_points.size());
        for (const auto& pt : filtered_points) {
            float x, y;
            core::MathUtils::polarToCartesian(pt.range, pt.angle, x, y);
            serialized_points.push_back({x, y, pt.range, pt.angle, pt.intensity});
        }
        
        history_frames_.push_back(serialized_points);
        size_t max_history = state_.enable_history ? static_cast<size_t>(state_.history_length) : 1;
        while (history_frames_.size() > max_history) {
            history_frames_.pop_front();
        }

        std::vector<rendering::RenderVertex> render_points;
        for (size_t i = 0; i < history_frames_.size(); ++i) {
            // Newest frame is at back (i = size-1), oldest at front (i = 0)
            // Wait, previous code had i=0 as age 0. If front is oldest, its age should be 1.0!
            // Let's fix age mapping: Newest frame -> age 0.0. Oldest -> age 1.0.
            float age = 0.0f;
            if (history_frames_.size() > 1) {
                age = 1.0f - (static_cast<float>(i) / static_cast<float>(history_frames_.size() - 1));
            }
            
            for (const auto& pt : history_frames_[i]) {
                render_points.push_back({pt.x, pt.y, pt.intensity, pt.range, age});
            }
        }
        
        gl_renderer_.updatePointCloud(render_points);

        if (state_.is_logging) {
            pipeline::LogFrame frame;
            static uint64_t current_frame_index = 0;
            frame.header.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            frame.header.frame_index = current_frame_index++;
            frame.header.motor_rpm = state_.current_rpm;
            frame.header.r_inst = state_.r_inst;
            frame.header.r_smooth = state_.r_smooth;
            frame.header.delta_r = auto_ranger_.getDeltaR();
            frame.header.alpha_adaptive = state_.alpha_adaptive;
            frame.header.point_count = static_cast<uint32_t>(serialized_points.size());
            frame.points = serialized_points;
            
            async_logger_.enqueueFrame(frame);
        }
    }

    // Update time history
    float delta_time = ImGui::GetIO().DeltaTime;
    state_.current_time += delta_time;
    if (state_.is_logging) {
        state_.log_duration += delta_time;
    }
    
    // Decimate history updates to e.g. 10 Hz
    static float history_timer = 0.0f;
    history_timer += delta_time;
    if (history_timer >= 0.1f) {
        history_timer = 0.0f;
        state_.time_history.push_back(state_.current_time);
        state_.r_inst_history.push_back(state_.r_inst);
        state_.r_smooth_history.push_back(state_.r_smooth);
        state_.alpha_history.push_back(state_.alpha_adaptive);
        state_.point_history.push_back(static_cast<float>(state_.point_count));
        
        if (state_.time_history.size() > 500) {
            state_.time_history.erase(state_.time_history.begin());
            state_.r_inst_history.erase(state_.r_inst_history.begin());
            state_.r_smooth_history.erase(state_.r_smooth_history.begin());
            state_.alpha_history.erase(state_.alpha_history.begin());
            state_.point_history.erase(state_.point_history.begin());
        }
    }
}

void AppWindow::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    setupDockspace();

    control_panel_.render(state_, lidar_controller_, signal_filter_, auto_ranger_);
    telemetry_panel_.render(state_);
    logging_panel_.render(state_, async_logger_);
    viewport_panel_.render(state_, gl_renderer_);
    replay_panel_.render(state_, gl_renderer_);

    ImGui::Render();
}

void AppWindow::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        processScanData();

        renderUI();

        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window_);
    }
}

} // namespace ols::ui
