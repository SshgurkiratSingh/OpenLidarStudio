#include <open_lidar_studio/core/math_utils.hpp>
#include <cmath>

namespace ols::core {

void MathUtils::polarToCartesian(float r, float theta_rad, float& x, float& y) {
    x = r * std::cos(theta_rad);
    y = r * std::sin(theta_rad);
}

float MathUtils::degreesToRadians(float degrees) {
    constexpr float kPi = 3.14159265358979323846f;
    return degrees * (kPi / 180.0f);
}

float MathUtils::radiansToDegrees(float radians) {
    constexpr float kPi = 3.14159265358979323846f;
    return radians * (180.0f / kPi);
}

} // namespace ols::core
