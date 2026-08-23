#include <open_lidar_studio/pipeline/signal_filter.hpp>
#include <open_lidar_studio/core/math_utils.hpp>
#include <cmath>

namespace ols::pipeline {

SignalFilter::SignalFilter() = default;

void SignalFilter::setRadiusBounds(float r_min, float r_max) {
    r_min_ = r_min;
    r_max_ = r_max;
}

void SignalFilter::setIntensityThreshold(float i_thresh) {
    i_threshold_ = i_thresh;
}

void SignalFilter::setAngularCullRegion(float theta_start_deg, float theta_end_deg) {
    cull_start_deg_ = theta_start_deg;
    cull_end_deg_ = theta_end_deg;
}

void SignalFilter::process(const std::vector<LaserPoint>& input, std::vector<LaserPoint>& output) const {
    output.clear();
    output.reserve(input.size());

    for (const auto& point : input) {
        float r = point.range;
        if (r < r_min_ || r > r_max_) continue;

        float intensity = point.intensity;
        if (intensity < i_threshold_) continue;

        float angle_deg = core::MathUtils::radiansToDegrees(point.angle);
        // Normalize angle to [0, 360) for simpler check
        while (angle_deg < 0) angle_deg += 360.0f;
        while (angle_deg >= 360.0f) angle_deg -= 360.0f;

        bool culled = false;
        if (cull_start_deg_ <= cull_end_deg_) {
            if (angle_deg >= cull_start_deg_ && angle_deg <= cull_end_deg_) {
                culled = true;
            }
        } else {
            // Wraps around 360
            if (angle_deg >= cull_start_deg_ || angle_deg <= cull_end_deg_) {
                culled = true;
            }
        }

        if (!culled) {
            output.push_back(point);
        }
    }
}

} // namespace ols::pipeline
