#include <open_lidar_studio/ui/app_window.hpp>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    std::cout << "Starting OpenLidarStudio..." << std::endl;

    auto app = std::make_unique<ols::ui::AppWindow>(1280, 720, "OpenLidarStudio");

    if (!app->initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }

    app->run();

    return 0;
}
