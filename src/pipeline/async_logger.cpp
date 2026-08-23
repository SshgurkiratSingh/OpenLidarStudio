#include <open_lidar_studio/pipeline/async_logger.hpp>
#include <chrono>
#include <iostream>

namespace ols::pipeline {

AsyncLogger::AsyncLogger(size_t buffer_size)
    : ring_buffer_(buffer_size) {
    writer_thread_ = std::thread(&AsyncLogger::writerLoop, this);
}

AsyncLogger::~AsyncLogger() {
    stopLogging();
    should_exit_ = true;
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }
}

void AsyncLogger::startLogging(const std::string& base_filename) {
    if (logging_) return;

    meta_stream_.open(base_filename + ".ols_meta", std::ios::binary | std::ios::trunc);
    data_stream_.open(base_filename + ".ols_data", std::ios::binary | std::ios::trunc);
    
    if (meta_stream_.is_open() && data_stream_.is_open()) {
        logging_ = true;
    }
}

void AsyncLogger::stopLogging() {
    logging_ = false;
    // We let the writer thread drain the buffer and then it will close the files
}

bool AsyncLogger::isLogging() const {
    return logging_;
}

bool AsyncLogger::enqueueFrame(const LogFrame& frame) {
    if (!logging_) return false;
    return ring_buffer_.push(frame);
}

void AsyncLogger::writerLoop() {
    LogFrame frame;
    while (!should_exit_) {
        bool has_data = false;
        
        while (ring_buffer_.pop(frame)) {
            has_data = true;
            if (meta_stream_.is_open() && data_stream_.is_open()) {
                meta_stream_.write(reinterpret_cast<const char*>(&frame.header), sizeof(core::TelemetryHeader));
                size_t points_size = frame.points.size() * sizeof(core::SerializedPoint);
                data_stream_.write(reinterpret_cast<const char*>(frame.points.data()), points_size);
            }
        }
        
        if (!logging_ && meta_stream_.is_open()) {
            meta_stream_.close();
            data_stream_.close();
        }

        if (!has_data) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    if (meta_stream_.is_open()) meta_stream_.close();
    if (data_stream_.is_open()) data_stream_.close();
}

bool AsyncLogger::exportToCSV(const std::string& meta_file, const std::string& data_file, const std::string& csv_out) {
    std::ifstream meta(meta_file, std::ios::binary);
    std::ifstream data(data_file, std::ios::binary);
    std::ofstream out(csv_out);
    
    if (!meta.is_open() || !data.is_open() || !out.is_open()) return false;
    
    out << "timestamp_ns,frame_index,motor_rpm,r_inst,r_smooth,delta_r,alpha_adaptive,x,y,range,angle,intensity\n";
    
    core::TelemetryHeader header;
    while (meta.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        std::vector<core::SerializedPoint> points(header.point_count);
        if (data.read(reinterpret_cast<char*>(points.data()), header.point_count * sizeof(core::SerializedPoint))) {
            for (const auto& pt : points) {
                out << header.timestamp_ns << "," << header.frame_index << "," << header.motor_rpm << "," 
                    << header.r_inst << "," << header.r_smooth << "," << header.delta_r << "," 
                    << header.alpha_adaptive << "," << pt.x << "," << pt.y << "," << pt.range << "," 
                    << pt.angle << "," << pt.intensity << "\n";
            }
        }
    }
    return true;
}

bool AsyncLogger::exportToJSON(const std::string& meta_file, const std::string& data_file, const std::string& json_out) {
    std::ifstream meta(meta_file, std::ios::binary);
    std::ifstream data(data_file, std::ios::binary);
    std::ofstream out(json_out);
    
    if (!meta.is_open() || !data.is_open() || !out.is_open()) return false;
    
    out << "[\n";
    bool first = true;
    core::TelemetryHeader header;
    while (meta.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        std::vector<core::SerializedPoint> points(header.point_count);
        if (data.read(reinterpret_cast<char*>(points.data()), header.point_count * sizeof(core::SerializedPoint))) {
            if (!first) out << ",\n";
            first = false;
            out << "  {\n";
            out << "    \"header\": {\"timestamp_ns\": " << header.timestamp_ns << ", \"frame_index\": " << header.frame_index 
                << ", \"motor_rpm\": " << header.motor_rpm << ", \"r_inst\": " << header.r_inst << ", \"point_count\": " << header.point_count << "},\n";
            out << "    \"points\": [\n";
            for (size_t i = 0; i < points.size(); ++i) {
                out << "      {\"x\": " << points[i].x << ", \"y\": " << points[i].y << ", \"range\": " << points[i].range 
                    << ", \"angle\": " << points[i].angle << ", \"intensity\": " << points[i].intensity << "}" << (i < points.size() - 1 ? "," : "") << "\n";
            }
            out << "    ]\n  }";
        }
    }
    out << "\n]\n";
    return true;
}

} // namespace ols::pipeline
