#include <open_lidar_studio/rendering/gl_renderer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace ols::rendering {

GLRenderer::GLRenderer() : pointcloud_shader_(std::make_unique<Shader>()) {
}

GLRenderer::~GLRenderer() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
}

bool GLRenderer::initialize() {
    if (!pointcloud_shader_->loadFromFile("assets/shaders/pointcloud.vert", "assets/shaders/pointcloud.frag")) {
        std::cerr << "Failed to load pointcloud shaders" << std::endl;
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Persistent allocation sized for max_vertices_
    glBufferData(GL_ARRAY_BUFFER, max_vertices_ * sizeof(RenderVertex), nullptr, GL_DYNAMIC_DRAW);

    // Position (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, x));
    glEnableVertexAttribArray(0);
    // Intensity
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, intensity));
    glEnableVertexAttribArray(1);
    // Range
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, range));
    glEnableVertexAttribArray(2);
    // Age
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, age));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return true;
}

void GLRenderer::resize(int width, int height) {
    viewport_width_ = width;
    viewport_height_ = height > 0 ? height : 1;
}

void GLRenderer::setMaxRange(float max_range) {
    max_range_ = max_range;
}

void GLRenderer::setColorMode(int mode) {
    color_mode_ = mode;
}

void GLRenderer::setHistoryDecay(float decay) {
    history_decay_ = decay;
}

void GLRenderer::updatePointCloud(const std::vector<RenderVertex>& points) {
    current_vertex_count_ = std::min(points.size(), max_vertices_);
    if (current_vertex_count_ == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, current_vertex_count_ * sizeof(RenderVertex), points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLRenderer::render() {
    if (current_vertex_count_ == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    pointcloud_shader_->use();

    float aspect = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
    float r = max_range_ > 0.1f ? max_range_ : 0.1f;
    glm::mat4 projection = glm::ortho(-r * aspect, r * aspect, -r, r, -1.0f, 1.0f);
    
    pointcloud_shader_->setMat4("projection", projection);
    pointcloud_shader_->setFloat("maxRange", max_range_);
    pointcloud_shader_->setInt("colorMode", color_mode_);
    pointcloud_shader_->setFloat("historyDecay", history_decay_);

    glBindVertexArray(vao_);
    glPointSize(3.0f); // Make points visible
    glDrawArrays(GL_POINTS, 0, current_vertex_count_);
    glBindVertexArray(0);
}

} // namespace ols::rendering
