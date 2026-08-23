#include <open_lidar_studio/ui/app_window.hpp>
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Starting OpenLidarStudio..." << std::endl;

    ols::ui::AppWindow app(1280, 720, "OpenLidarStudio");

    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }

    app.run();

    return 0;
}
