#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ols::rendering {

class Shader {
public:
    Shader();
    ~Shader();

    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;

    // Utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    GLuint getID() const { return ID; }

private:
    GLuint ID{0};
    void checkCompileErrors(GLuint shader, const std::string& type) const;
};

} // namespace ols::rendering
