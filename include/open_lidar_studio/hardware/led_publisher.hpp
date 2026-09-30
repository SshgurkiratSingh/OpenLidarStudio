#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>

namespace ols::hardware {

class LedPublisher {
public:
    LedPublisher();
    ~LedPublisher();
    
    // Starts the background threads for discovery and streaming
    void start();
    void stop();
    
    // Pushes the latest RGB buffer to be transmitted (thread-safe)
    void setRGBBuffer(const std::vector<uint8_t>& buffer);
    
    // Status accessors for the UI
    std::string getDiscoveredIP();
    std::string getStatusMessage();

private:
    std::atomic<bool> running_{false};
    std::thread discovery_thread_;
    std::thread stream_thread_;
    
    std::mutex data_mutex_;
    std::vector<uint8_t> current_buffer_;
    
    std::mutex status_mutex_;
    std::string discovered_ip_{""};
    std::string status_message_{"Not connected"};
    
    void discoveryLoop();
    void streamLoop();
};

} // namespace ols::hardware
