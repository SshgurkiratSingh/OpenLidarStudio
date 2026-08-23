#pragma once

#include <string>
#include <vector>

namespace ols::hardware {

class SerialEnumerator {
public:
    static std::vector<std::string> getAvailablePorts();
};

} // namespace ols::hardware
