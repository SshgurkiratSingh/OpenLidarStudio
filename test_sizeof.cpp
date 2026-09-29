#include <iostream>
#include "include/open_lidar_studio/ui/app_window.hpp"
int main() {
    std::cout << "Size of AppWindow: " << sizeof(ols::ui::AppWindow) << std::endl;
    std::cout << "Size of UIState: " << sizeof(ols::ui::UIState) << std::endl;
    return 0;
}
