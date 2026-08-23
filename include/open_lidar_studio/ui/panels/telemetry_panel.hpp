#pragma once

#include <open_lidar_studio/ui/ui_state.hpp>

namespace ols::ui {

class TelemetryPanel {
public:
    TelemetryPanel();
    ~TelemetryPanel() = default;

    void render(UIState& state);
};

} // namespace ols::ui
