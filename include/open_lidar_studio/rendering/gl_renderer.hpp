#pragma once

#include <open_lidar_studio/rendering/shader.hpp>
#include <open_lidar_studio/core/telemetry_types.hpp>
#include <glad/glad.h>
#include <vector>
#include <memory>

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
    void setColorMode(int mode); // 0 = Range, 1 = Intensity
    void setHistoryDecay(float decay);

    void updatePointCloud(const std::vector<RenderVertex>& points);
    void render();

private:
    std::unique_ptr<Shader> pointcloud_shader_;
    
    GLuint vao_{0};
    GLuint vbo_{0};
    size_t max_vertices_{32768};
    size_t current_vertex_count_{0};

    int viewport_width_{800};
    int viewport_height_{600};
    float max_range_{10.0f};
    int color_mode_{0};
    float history_decay_{1.5f};
};

} // namespace ols::rendering
