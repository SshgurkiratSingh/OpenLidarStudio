#pragma once

namespace ols::core {

class MathUtils {
public:
    static void polarToCartesian(float r, float theta_rad, float& x, float& y);
    static float degreesToRadians(float degrees);
    static float radiansToDegrees(float radians);
};

} // namespace ols::core
