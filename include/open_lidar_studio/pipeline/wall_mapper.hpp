#pragma once

#include <vector>
#include <cstdint>
#include <CYdLidar.h>
#include <open_lidar_studio/ui/ui_state.hpp>

namespace ols::pipeline {

class WallMapper {
public:
    WallMapper();
    
    // Updates the internal RGB buffer based on tracked objects, zones, and idle time
    void process(const ui::UIState& state, const std::vector<LaserPoint>& points, float dt);
    
    // Returns the generated RGB buffer (size = led_count * 3)
    const std::vector<uint8_t>& getRGBBuffer() const { return rgb_buffer_; }

private:
    std::vector<uint8_t> rgb_buffer_;
    float animation_time_{0.0f};
    
    void applyIdleAnimation(const ui::UIState& state);
    void applyTrackingGlow(const ui::UIState& state);
    void applyRawPointsGlow(const ui::UIState& state, const std::vector<LaserPoint>& points);
    void applyZoneAlert(const ui::UIState& state);
    
    // Helper to blend a color into an LED index
    void blendLED(int index, uint8_t r, uint8_t g, uint8_t b, float alpha);
    
    // HSB to RGB helper
    void hsv2rgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b);
};

} // namespace ols::pipeline
