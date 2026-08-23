#include <open_lidar_studio/pipeline/auto_ranger.hpp>
#include <cmath>
#include <algorithm>

namespace ols::pipeline {

AutoRanger::AutoRanger() = default;

void AutoRanger::setMode(bool is_auto) {
    auto_mode_ = is_auto;
}

void AutoRanger::setManualRange(float range) {
    manual_range_ = range;
}

void AutoRanger::setParameters(float alpha_min, float alpha_max, float k_sensitivity, float hysteresis_gamma) {
    alpha_min_ = alpha_min;
    alpha_max_ = alpha_max;
    k_sensitivity_ = k_sensitivity;
    gamma_ = hysteresis_gamma;
}

void AutoRanger::process(const std::vector<LaserPoint>& points) {
    if (!auto_mode_) {
        r_final_ = manual_range_;
        return;
    }

    if (points.empty()) {
        r_final_ = r_smooth_prev_ * gamma_;
        return;
    }

    r_inst_ = 0.0f;
    for (const auto& pt : points) {
        if (pt.range > r_inst_) {
            r_inst_ = pt.range;
        }
    }

    delta_r_ = std::abs(r_inst_ - r_smooth_prev_);
    
    alpha_t_ = alpha_min_ + (alpha_max_ - alpha_min_) * (1.0f - std::exp(-delta_r_ / k_sensitivity_));
    
    r_smooth_ = alpha_t_ * r_inst_ + (1.0f - alpha_t_) * r_smooth_prev_;
    
    r_final_ = r_smooth_ * gamma_;
    
    r_smooth_prev_ = r_smooth_;
}

float AutoRanger::getFinalRange() const { return auto_mode_ ? r_final_ : manual_range_; }
float AutoRanger::getInstRange() const { return r_inst_; }
float AutoRanger::getSmoothRange() const { return r_smooth_; }
float AutoRanger::getDeltaR() const { return delta_r_; }
float AutoRanger::getAlphaAdaptive() const { return alpha_t_; }

} // namespace ols::pipeline
