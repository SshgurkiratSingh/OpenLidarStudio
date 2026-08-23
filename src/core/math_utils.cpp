#include <open_lidar_studio/core/math_utils.hpp>
#include <cmath>

namespace ols::core {

void MathUtils::polarToCartesian(float r, float theta_rad, float& x, float& y) {
    x = r * std::cos(theta_rad);
    y = r * std::sin(theta_rad);
}

float MathUtils::degreesToRadians(float degrees) {
    return degrees * (static_cast<float>(M_PI) / 180.0f);
}

float MathUtils::radiansToDegrees(float radians) {
    return radians * (180.0f / static_cast<float>(M_PI));
}

} // namespace ols::core
