#pragma once

#include <cstdint>

namespace ols::core {

#pragma pack(push, 1)
struct TelemetryHeader {
    uint64_t timestamp_ns;
    uint32_t frame_index;
    float motor_rpm;
    float r_inst;
    float r_smooth;
    float delta_r;
    float alpha_adaptive;
    uint32_t point_count;
};

struct SerializedPoint {
    float x;
    float y;
    float range;
    float angle;
    float intensity;
};
#pragma pack(pop)

} // namespace ols::core
