#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <CYdLidar.h>
#include <open_lidar_studio/core/telemetry_types.hpp>

namespace ols::hardware {

class LidarController {
public:
    LidarController();
    ~LidarController();

    bool connect(const std::string& port, int baudrate, bool single_channel);
    void disconnect();
    bool isConnected() const;

    void startScan();
    void stopScan();
    bool isScanning() const;

    void setMotorFrequency(float hz); // 5.0 to 15.0 Hz
    float getMotorFrequency() const;

    void setScanFrequencyAndSampleRate(float scan_freq, int sample_rate);
    void setAngularBounds(float min_angle_deg, float max_angle_deg);

    // Set callback to receive raw scans from the worker thread
    using ScanCallback = std::function<void(const LaserScan&)>;
    void setScanCallback(ScanCallback callback);

private:
    void workerLoop();

    std::unique_ptr<CYdLidar> lidar_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> scanning_{false};
    std::atomic<bool> should_exit_{false};

    std::thread worker_thread_;
    ScanCallback scan_callback_;
    float current_motor_hz_{10.0f};
};

} // namespace ols::hardware
