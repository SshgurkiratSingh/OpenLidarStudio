#include <open_lidar_studio/hardware/led_publisher.hpp>
#include <iostream>
#include <chrono>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace ols::hardware {

LedPublisher::LedPublisher() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

LedPublisher::~LedPublisher() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void LedPublisher::start() {
    if (running_) return;
    running_ = true;
    
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        status_message_ = "Searching for ESP32 beacon on port 5000...";
    }
    
    discovery_thread_ = std::thread(&LedPublisher::discoveryLoop, this);
    stream_thread_ = std::thread(&LedPublisher::streamLoop, this);
}

void LedPublisher::stop() {
    running_ = false;
    if (discovery_thread_.joinable()) discovery_thread_.join();
    if (stream_thread_.joinable()) stream_thread_.join();
}

void LedPublisher::setRGBBuffer(const std::vector<uint8_t>& buffer) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    current_buffer_ = buffer;
}

std::string LedPublisher::getDiscoveredIP() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return discovered_ip_;
}

std::string LedPublisher::getStatusMessage() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return status_message_;
}

void LedPublisher::discoveryLoop() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    DWORD timeout = 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct timeval tv;
    tv.tv_sec = 1; // 1 second timeout
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5000); // Listen on 5000
    
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "LedPublisher: Failed to bind discovery socket" << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return;
    }
    
    char buffer[256];
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) {
            buffer[n] = '\0';
            std::string msg(buffer);
            if (msg.find("ESP32_LED_NODE") != std::string::npos) {
                std::string ip = inet_ntoa(client_addr.sin_addr);
                std::lock_guard<std::mutex> lock(status_mutex_);
                if (discovered_ip_ != ip) {
                    discovered_ip_ = ip;
                    status_message_ = "Streaming to " + ip + ":5001";
                }
            }
        }
    }
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void LedPublisher::streamLoop() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    
    while (running_) {
        std::string ip;
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            ip = discovered_ip_;
        }
        
        std::vector<uint8_t> payload;
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            payload = current_buffer_;
        }
        
        if (!ip.empty() && !payload.empty()) {
            struct sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(5001); // Stream to 5001
            inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr);
            
            // Note: If payload is large (>1400 bytes, e.g., >460 LEDs), UDP fragmentation occurs.
            // For 300 LEDs (900 bytes), it fits in one MTU packet perfectly.
            sendto(sock, payload.data(), payload.size(), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 Hz
    }
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

} // namespace ols::hardware
