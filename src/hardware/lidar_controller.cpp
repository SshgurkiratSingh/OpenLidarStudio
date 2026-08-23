#include <open_lidar_studio/hardware/lidar_controller.hpp>
#include <iostream>

namespace ols::hardware {

LidarController::LidarController() : lidar_(std::make_unique<CYdLidar>()) {
}

LidarController::~LidarController() {
    disconnect();
}

bool LidarController::connect(const std::string& port, int baudrate, bool single_channel) {
    disconnect();
    
    lidar_ = std::make_unique<CYdLidar>();
    lidar_->setSerialPort(port);
    lidar_->setSerialBaudrate(baudrate);
    lidar_->setLidarType(TYPE_TRIANGLE);
    lidar_->setScanFrequency(5.5f);
    lidar_->setSampleRate(6);
    lidar_->setSingleChannel(single_channel);

    if (lidar_->initialize()) {
        connected_ = true;
        should_exit_ = false;
        worker_thread_ = std::thread(&LidarController::workerLoop, this);
        return true;
    }
    
    return false;
}

void LidarController::disconnect() {
    if (!connected_) return;

    should_exit_ = true;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    lidar_->turnOff();
    lidar_->disconnecting();
    connected_ = false;
    scanning_ = false;
}

bool LidarController::isConnected() const {
    return connected_;
}

void LidarController::startScan() {
    if (connected_ && !scanning_) {
        lidar_->turnOn();
        scanning_ = true;
    }
}

void LidarController::stopScan() {
    if (scanning_) {
        lidar_->turnOff();
        scanning_ = false;
    }
}

bool LidarController::isScanning() const {
    return scanning_;
}

void LidarController::setMotorFrequency(float hz) {
    current_motor_hz_ = hz; // In a real scenario we might need to set it via SDK
    // For many ydlidar models, there's a setMotorFrequency or similar prop
}

float LidarController::getMotorFrequency() const {
    return current_motor_hz_; // Dummy for now
}

void LidarController::setScanFrequencyAndSampleRate(float scan_freq, int sample_rate) {
    lidar_->setScanFrequency(scan_freq);
    lidar_->setSampleRate(sample_rate);
}

void LidarController::setAngularBounds(float min_angle_deg, float max_angle_deg) {
    lidar_->setMinAngle(min_angle_deg);
    lidar_->setMaxAngle(max_angle_deg);
}

void LidarController::setScanCallback(ScanCallback callback) {
    scan_callback_ = callback;
}

void LidarController::workerLoop() {
    while (!should_exit_) {
        if (scanning_) {
            LaserScan scan;
            bool hwError;
            if (lidar_->doProcessSimple(scan, hwError)) {
                if (scan_callback_) {
                    scan_callback_(scan);
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

} // namespace ols::hardware
