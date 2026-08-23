#pragma once

#include <vector>
#include <CYdLidar.h>

namespace ols::pipeline {

class AutoRanger {
public:
    AutoRanger();

    void setMode(bool is_auto);
    void setManualRange(float range);
    void setParameters(float alpha_min, float alpha_max, float k_sensitivity, float hysteresis_gamma);

    void process(const std::vector<LaserPoint>& points);

    float getFinalRange() const;
    float getInstRange() const;
    float getSmoothRange() const;
    float getDeltaR() const;
    float getAlphaAdaptive() const;

private:
    bool auto_mode_{true};
    float manual_range_{10.0f};

    float alpha_min_{0.05f};
    float alpha_max_{0.40f};
    float k_sensitivity_{1.50f};
    float gamma_{1.15f};

    float r_inst_{0.0f};
    float r_smooth_prev_{10.0f};
    float r_smooth_{10.0f};
    float delta_r_{0.0f};
    float alpha_t_{0.05f};
    float r_final_{10.0f};
};

} // namespace ols::pipeline
