#include <open_lidar_studio/hardware/serial_enumerator.hpp>
#include <CYdLidar.h>
#include <map>

namespace ols::hardware {

std::vector<std::string> SerialEnumerator::getAvailablePorts() {
    std::vector<std::string> ports;
    std::map<std::string, std::string> ydlidar_ports = ydlidar::YDlidarDriver::lidarPortList();
    
    for (const auto& [port_name, description] : ydlidar_ports) {
        ports.push_back(port_name);
    }
    
    return ports;
}

} // namespace ols::hardware
