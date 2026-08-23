#pragma once

#include <open_lidar_studio/rendering/shader.hpp>
#include <open_lidar_studio/core/telemetry_types.hpp>
#include <glad/glad.h>
#include <vector>
#include <memory>
#include <string>

namespace ols::rendering {

struct RenderVertex {
    float x;
    float y;
    float intensity;
    float range;
    float age;
};

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool initialize();
    void resize(int width, int height);

    void setMaxRange(float max_range);
    void setColorMode(int mode);        // 0 = Range, 1 = Intensity
    void setHistoryDecay(float decay);
    void setPointSize(float size);
    void setCircularPoints(bool circular);
    void setZoomPan(float zoom, float pan_x, float pan_y);

    void updatePointCloud(const std::vector<RenderVertex>& points);
    void render();

    // Screenshot helper: reads current FBO pixels into an RGBA buffer
    // Returns true on success; caller must free with delete[].
    bool readPixels(std::vector<unsigned char>& out_rgba) const;
    int getWidth() const { return viewport_width_; }
    int getHeight() const { return viewport_height_; }

private:
    std::unique_ptr<Shader> pointcloud_shader_;
    
    GLuint vao_{0};
    GLuint vbo_{0};
    size_t max_vertices_{65536};
    size_t current_vertex_count_{0};

    int viewport_width_{800};
    int viewport_height_{600};
    float max_range_{10.0f};
    int color_mode_{0};
    float history_decay_{1.5f};
    float point_size_{4.0f};
    bool circular_points_{true};
    float zoom_{1.0f};
    float pan_x_{0.0f};
    float pan_y_{0.0f};
};

} // namespace ols::rendering
