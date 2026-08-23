#pragma once

#include <open_lidar_studio/core/telemetry_types.hpp>
#include <open_lidar_studio/core/ring_buffer.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <fstream>
#include <vector>

namespace ols::pipeline {

struct LogFrame {
    core::TelemetryHeader header;
    std::vector<core::SerializedPoint> points;
};

class AsyncLogger {
public:
    AsyncLogger(size_t buffer_size = 1024);
    ~AsyncLogger();

    void startLogging(const std::string& base_filename);
    void stopLogging();
    bool isLogging() const;
    
    // Non-blocking push for the producer thread
    bool enqueueFrame(const LogFrame& frame);

    // Export utilities
    static bool exportToCSV(const std::string& meta_file, const std::string& data_file, const std::string& csv_out);
    static bool exportToJSON(const std::string& meta_file, const std::string& data_file, const std::string& json_out);

private:
    void writerLoop();

    core::RingBuffer<LogFrame> ring_buffer_;
    std::atomic<bool> logging_{false};
    std::atomic<bool> should_exit_{false};
    std::thread writer_thread_;

    std::ofstream meta_stream_;
    std::ofstream data_stream_;
};

} // namespace ols::pipeline
