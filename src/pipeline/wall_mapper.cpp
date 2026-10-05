#include <open_lidar_studio/pipeline/wall_mapper.hpp>
#include <open_lidar_studio/core/math_utils.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstdlib>

namespace ols::pipeline {

WallMapper::WallMapper() {}

void WallMapper::hsv2rgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float h_prime = std::fmod(h / 60.0f, 6.0f);
    float x = c * (1.0f - std::abs(std::fmod(h_prime, 2.0f) - 1.0f));
    float m = v - c;
    
    float rf = 0, gf = 0, bf = 0;
    if (h_prime >= 0 && h_prime < 1) { rf = c; gf = x; bf = 0; }
    else if (h_prime >= 1 && h_prime < 2) { rf = x; gf = c; bf = 0; }
    else if (h_prime >= 2 && h_prime < 3) { rf = 0; gf = c; bf = x; }
    else if (h_prime >= 3 && h_prime < 4) { rf = 0; gf = x; bf = c; }
    else if (h_prime >= 4 && h_prime < 5) { rf = x; gf = 0; bf = c; }
    else if (h_prime >= 5 && h_prime < 6) { rf = c; gf = 0; bf = x; }
    
    r = static_cast<uint8_t>((rf + m) * 255.0f);
    g = static_cast<uint8_t>((gf + m) * 255.0f);
    b = static_cast<uint8_t>((bf + m) * 255.0f);
}

void WallMapper::blendLED(int index, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (index < 0 || index * 3 + 2 >= (int)rgb_buffer_.size()) return;
    int i = index * 3;
    rgb_buffer_[i] = static_cast<uint8_t>(rgb_buffer_[i] * (1.0f - alpha) + r * alpha);
    rgb_buffer_[i+1] = static_cast<uint8_t>(rgb_buffer_[i+1] * (1.0f - alpha) + g * alpha);
    rgb_buffer_[i+2] = static_cast<uint8_t>(rgb_buffer_[i+2] * (1.0f - alpha) + b * alpha);
}

void WallMapper::process(const ui::UIState& state, const std::vector<LaserPoint>& points, float dt) {
    if (!state.led_wall.enabled || !state.led_wall.define_point_a_set || !state.led_wall.define_point_b_set) {
        rgb_buffer_.clear();
        return;
    }
    
    int num_leds = state.led_wall.led_count;
    if (num_leds < 1) return;
    
    if (rgb_buffer_.size() != static_cast<size_t>(num_leds * 3)) {
        rgb_buffer_.assign(num_leds * 3, 0);
    }
    
    // Fade trail
    for (auto& val : rgb_buffer_) {
        val = static_cast<uint8_t>(val * 0.7f); // trail decay
    }
    
    animation_time_ += dt;
    
    bool is_zone_breached = false;
    for (const auto& zone : state.zones) {
        if (zone.enabled && zone.triggered) {
            is_zone_breached = true;
            break;
        }
    }
    
    bool is_anyone_tracked = false;
    for (const auto& obj : state.tracked_objects) {
        if (obj.is_confirmed) {
            is_anyone_tracked = true;
            break;
        }
    }
    
    bool has_close_points = false;
    float ax = state.led_wall.ax;
    float ay = state.led_wall.ay;
    float bx = state.led_wall.bx;
    float by = state.led_wall.by;
    float wall_dx = bx - ax;
    float wall_dy = by - ay;
    float wall_len_sq = wall_dx * wall_dx + wall_dy * wall_dy;
    
    if (!state.led_wall.bypass_lidar && wall_len_sq > 0.0001f) {
        for (const auto& pt : points) {
            float px, py;
            core::MathUtils::polarToCartesian(pt.range, pt.angle, px, py);
            float dot = (px - ax) * wall_dx + (py - ay) * wall_dy;
            float t = dot / wall_len_sq;
            if (t >= -0.1f && t <= 1.1f) {
                float proj_x = ax + t * wall_dx;
                float proj_y = ay + t * wall_dy;
                float dist_sq = (px - proj_x)*(px - proj_x) + (py - proj_y)*(py - proj_y);
                if (dist_sq < 2.5f * 2.5f) {
                    has_close_points = true;
                    break;
                }
            }
        }
    }
    
    if (state.led_wall.bypass_lidar) {
        applyIdleAnimation(state);
    } else if (is_zone_breached) {
        applyZoneAlert(state);
    } else if (is_anyone_tracked) {
        applyTrackingGlow(state);
    } else if (has_close_points) {
        applyRawPointsGlow(state, points);
    } else {
        applyIdleAnimation(state);
    }
    
    // Invert LED output order if needed
    if (state.led_wall.invert_leds) {
        std::vector<uint8_t> inverted(num_leds * 3);
        for (int i = 0; i < num_leds; ++i) {
            int src_idx = i * 3;
            int dst_idx = (num_leds - 1 - i) * 3;
            inverted[dst_idx] = rgb_buffer_[src_idx];
            inverted[dst_idx + 1] = rgb_buffer_[src_idx + 1];
            inverted[dst_idx + 2] = rgb_buffer_[src_idx + 2];
        }
        rgb_buffer_ = std::move(inverted);
    }
}

void WallMapper::applyIdleAnimation(const ui::UIState& state) {
    int anim = state.led_wall.idle_animation;
    int num_leds = state.led_wall.led_count;
    
    if (anim == 1) { // Rainbow
        for (int i = 0; i < num_leds; ++i) {
            float hue = std::fmod(animation_time_ * 50.0f + (static_cast<float>(i) / num_leds) * 360.0f, 360.0f);
            uint8_t r, g, b;
            hsv2rgb(hue, 1.0f, 0.5f, r, g, b); // 50% brightness
            blendLED(i, r, g, b, 1.0f);
        }
    } else if (anim == 2) { // Breathe
        float val = (std::sin(animation_time_ * 2.0f) + 1.0f) * 0.5f * 0.5f; // 0 to 0.5 brightness
        uint8_t r = static_cast<uint8_t>(0 * val * 255);
        uint8_t g = static_cast<uint8_t>(200 * val);
        uint8_t b = static_cast<uint8_t>(255 * val);
        for (int i = 0; i < num_leds; ++i) {
            blendLED(i, r, g, b, 1.0f);
        }
    } else if (anim == 3) { // Theater Chase
        int step = static_cast<int>(animation_time_ * 10.0f) % 3;
        for (int i = 0; i < num_leds; ++i) {
            if ((i + step) % 3 == 0) {
                blendLED(i, 150, 150, 255, 1.0f);
            } else {
                blendLED(i, 0, 0, 0, 1.0f);
            }
        }
    } else if (anim == 4) { // Sparkle
        for (int i = 0; i < num_leds; ++i) {
            blendLED(i, 10, 0, 20, 1.0f); // dim purple background
        }
        int sparkles = (std::max)(1, num_leds / 20);
        for (int i = 0; i < sparkles; ++i) {
            if (std::rand() % 10 == 0) { // randomness threshold
                int idx = std::rand() % num_leds;
                blendLED(idx, 255, 255, 255, 1.0f);
            }
        }
    } else if (anim == 5) { // Scanner
        float pos = (std::sin(animation_time_ * 4.0f) + 1.0f) * 0.5f * (num_leds - 1);
        for (int i = 0; i < num_leds; ++i) {
            blendLED(i, 0, 0, 0, 1.0f); // clear background
        }
        int glow_size = (std::max)(2, num_leds / 10);
        for (int i = -glow_size; i <= glow_size; ++i) {
            int idx = static_cast<int>(pos) + i;
            if (idx >= 0 && idx < num_leds) {
                float intensity = 1.0f - (std::abs(static_cast<float>(i)) / glow_size);
                blendLED(idx, 255, 0, 0, intensity);
            }
        }
    } else if (anim == 6) { // Fire
        static std::vector<float> heat(num_leds, 0.0f);
        if (heat.size() != static_cast<size_t>(num_leds)) heat.assign(num_leds, 0.0f);
        
        // Cool down every cell
        for (int i = 0; i < num_leds; i++) {
            float cooldown = static_cast<float>(std::rand() % 100) / 100.0f * 0.1f;
            heat[i] = (std::max)(0.0f, heat[i] - cooldown);
        }
        
        // Heat from each cell drifts 'up' (to higher indices)
        for (int i = num_leds - 1; i >= 2; i--) {
            heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3.0f;
        }
        
        // Randomly ignite sparks near the bottom
        if (std::rand() % 10 < 5) {
            int y = std::rand() % (std::max)(2, num_leds / 10);
            heat[y] = heat[y] + static_cast<float>(std::rand() % 100) / 100.0f * 0.5f + 0.5f;
            heat[y] = (std::min)(1.0f, heat[y]);
        }
        
        // Map heat to color (0=black, 0.5=red, 0.8=yellow, 1.0=white)
        for (int i = 0; i < num_leds; i++) {
            float h = heat[i];
            uint8_t r = 0, g = 0, b = 0;
            if (h > 0.8f) { // white/yellow
                r = 255; g = 255; b = static_cast<uint8_t>((h - 0.8f) * 5.0f * 255);
            } else if (h > 0.4f) { // red/yellow
                r = 255; g = static_cast<uint8_t>((h - 0.4f) * 2.5f * 255); b = 0;
            } else { // black/red
                r = static_cast<uint8_t>(h * 2.5f * 255); g = 0; b = 0;
            }
            blendLED(i, r, g, b, 1.0f);
        }
    } else if (anim == 7) { // Meteor Rain
        for (int i = 0; i < num_leds; ++i) {
            blendLED(i, 0, 0, 0, 0.2f); // slow fade to black for trails
        }
        int pos = static_cast<int>(animation_time_ * 80.0f) % (num_leds + 50);
        for (int i = 0; i < 5; i++) {
            if (pos - i >= 0 && pos - i < num_leds) {
                float intensity = 1.0f - (i / 5.0f);
                blendLED(pos - i, 200, 200, 255, intensity);
            }
        }
    } else if (anim == 8) { // Cyberpunk Pulse
        for (int i = 0; i < num_leds; ++i) {
            float wave1 = std::sin(animation_time_ * 3.0f + i * 0.1f);
            float wave2 = std::cos(animation_time_ * 4.0f - i * 0.15f);
            
            float magenta_val = (wave1 + 1.0f) * 0.5f;
            float cyan_val = (wave2 + 1.0f) * 0.5f;
            
            uint8_t r = static_cast<uint8_t>(255 * magenta_val);
            uint8_t g = static_cast<uint8_t>(255 * cyan_val);
            uint8_t b = 255; // Always full blue for that outrun vibe
            
            blendLED(i, r, g, b, 1.0f);
        }
    } else if (anim == 9) { // Lava
        for (int i = 0; i < num_leds; ++i) {
            float noise = std::sin(animation_time_ * 1.5f + i * 0.08f) + 
                          std::sin(animation_time_ * 0.8f - i * 0.04f);
            float normalized = (noise + 2.0f) * 0.25f; // 0.0 to 1.0
            
            // Warm colors: hue from 0 (red) to 45 (yellow/orange)
            float hue = normalized * 45.0f;
            uint8_t r, g, b;
            hsv2rgb(hue, 1.0f, 0.8f + (normalized * 0.2f), r, g, b); 
            blendLED(i, r, g, b, 1.0f);
        }
    } else if (anim == 10) { // Bioluminescence
        for (int i = 0; i < num_leds; ++i) {
            blendLED(i, 0, 5, 20, 1.0f); // Deep ocean blue background
        }
        
        static std::vector<float> phases(num_leds, 0.0f);
        if (phases.size() != static_cast<size_t>(num_leds)) {
            phases.assign(num_leds, 0.0f);
            for(int i=0; i<num_leds; i++) phases[i] = static_cast<float>(std::rand() % 100) / 100.0f * 3.14159f * 2.0f;
        }
        
        for (int i = 0; i < num_leds; ++i) {
            float intensity = (std::sin(animation_time_ * 1.0f + phases[i]) + 1.0f) * 0.5f;
            // Only show glow if it's high intensity, creating isolated nodes
            if (intensity > 0.8f) {
                float glow = (intensity - 0.8f) * 5.0f; // 0 to 1.0
                blendLED(i, 0, static_cast<uint8_t>(255*glow), static_cast<uint8_t>(200*glow), glow);
            }
        }
    }
}

void WallMapper::applyZoneAlert(const ui::UIState& state) {
    // Flash red rapidly
    bool flash = std::fmod(animation_time_, 0.2f) < 0.1f;
    uint8_t c = flash ? 255 : 0;
    int num_leds = state.led_wall.led_count;
    for (int i = 0; i < num_leds; ++i) {
        blendLED(i, c, 0, 0, 1.0f);
    }
}

void WallMapper::applyTrackingGlow(const ui::UIState& state) {
    int num_leds = state.led_wall.led_count;
    float ax = state.led_wall.ax;
    float ay = state.led_wall.ay;
    float bx = state.led_wall.bx;
    float by = state.led_wall.by;
    
    float wall_dx = bx - ax;
    float wall_dy = by - ay;
    float wall_len_sq = wall_dx * wall_dx + wall_dy * wall_dy;
    if (wall_len_sq < 0.0001f) return;
    float wall_len = std::sqrt(wall_len_sq);
    
    for (const auto& obj : state.tracked_objects) {
        if (!obj.is_confirmed) continue;
        
        // Vector from wall A to person
        float px = obj.center_x - ax;
        float py = obj.center_y - ay;
        
        // Project person onto wall
        float dot = px * wall_dx + py * wall_dy;
        float t = dot / wall_len_sq; // Normalized position along wall (0 to 1)
        
        // Calculate perpendicular distance from wall
        float proj_x = ax + t * wall_dx;
        float proj_y = ay + t * wall_dy;
        float dist_to_wall = std::sqrt((obj.center_x - proj_x)*(obj.center_x - proj_x) + 
                                       (obj.center_y - proj_y)*(obj.center_y - proj_y));
                                       
        // If they are way outside the bounds of the segment, ignore or clamp?
        // Let's clamp so they stick to the edges if they walk past
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        
        int center_led = static_cast<int>(t * num_leds);
        
        // Map distance to color (closer = hotter/redder, further = colder/bluer)
        // Let's say 0.5m is RED (0 hue), 2.0m is BLUE (240 hue)
        float hue = 0.0f;
        if (dist_to_wall > 0.5f) {
            float dist_clamped = std::min(dist_to_wall, 2.5f);
            hue = (dist_clamped - 0.5f) / 2.0f * 240.0f;
        }
        uint8_t r, g, b;
        hsv2rgb(hue, 1.0f, 1.0f, r, g, b); // Full brightness
        
        // Draw a glow (gaussian-ish)
        int glow_radius = std::max(2, static_cast<int>(num_leds * 0.05f)); // 5% of wall size
        for (int i = -glow_radius; i <= glow_radius; ++i) {
            int led = center_led + i;
            if (led >= 0 && led < num_leds) {
                float intensity = 1.0f - (std::abs(static_cast<float>(i)) / glow_radius);
                blendLED(led, r, g, b, intensity);
            }
        }
    }
}

void WallMapper::applyRawPointsGlow(const ui::UIState& state, const std::vector<LaserPoint>& points) {
    int num_leds = state.led_wall.led_count;
    float ax = state.led_wall.ax;
    float ay = state.led_wall.ay;
    float bx = state.led_wall.bx;
    float by = state.led_wall.by;
    
    float wall_dx = bx - ax;
    float wall_dy = by - ay;
    float wall_len_sq = wall_dx * wall_dx + wall_dy * wall_dy;
    if (wall_len_sq < 0.0001f) return;
    float wall_len = std::sqrt(wall_len_sq);
    
    for (const auto& pt : points) {
        float px, py;
        core::MathUtils::polarToCartesian(pt.range, pt.angle, px, py);
        
        float dot = (px - ax) * wall_dx + (py - ay) * wall_dy;
        float t = dot / wall_len_sq;
        
        if (t < -0.1f || t > 1.1f) continue;
        
        float proj_x = ax + t * wall_dx;
        float proj_y = ay + t * wall_dy;
        float dist_sq = (px - proj_x)*(px - proj_x) + (py - proj_y)*(py - proj_y);
        
        if (dist_sq < 2.5f * 2.5f) {
            float dist = std::sqrt(dist_sq);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            
            int center_led = static_cast<int>(t * num_leds);
            if (center_led >= num_leds) center_led = num_leds - 1;
            
            float hue = 0.0f;
            if (dist > 0.5f) {
                float dist_clamped = std::min(dist, 2.5f);
                hue = (dist_clamped - 0.5f) / 2.0f * 240.0f;
            }
            uint8_t r, g, b;
            hsv2rgb(hue, 1.0f, 1.0f, r, g, b);
            
            int glow_radius = std::max(1, static_cast<int>(num_leds * 0.02f));
            for (int i = -glow_radius; i <= glow_radius; ++i) {
                int led = center_led + i;
                if (led >= 0 && led < num_leds) {
                    float intensity = 1.0f - (std::abs(static_cast<float>(i)) / glow_radius);
                    blendLED(led, r, g, b, intensity * 0.4f);
                }
            }
        }
    }
}

} // namespace ols::pipeline
