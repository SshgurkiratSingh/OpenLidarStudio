#pragma once

#include <vector>
#include <CYdLidar.h>

namespace ols::pipeline {

class SignalFilter {
public:
    SignalFilter();

    void setRadiusBounds(float r_min, float r_max);
    void setIntensityThreshold(float i_thresh);
    void setAngularCullRegion(float theta_start_deg, float theta_end_deg);

    // Filters points in-place, returning a compacted vector of valid points
    void process(const std::vector<LaserPoint>& input, std::vector<LaserPoint>& output) const;

private:
    float r_min_{0.0f};
    float r_max_{100.0f}; // meters
    float i_threshold_{0.0f};
    float cull_start_deg_{0.0f};
    float cull_end_deg_{0.0f};
};

} // namespace ols::pipeline
