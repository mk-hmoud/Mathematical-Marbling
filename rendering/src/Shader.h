#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

/**
 * @class Shader
 * @brief A helper class to compile GLSL shaders and manage a shader program.
 *
 * This class handles loading shader source code from strings,
 * compiling them, linking them into a single program, and providing
 * utility functions to set shader "uniform" variables.
 */
class Shader
{
public:
    /**
     * @brief The public ID (handle) of the linked OpenGL shader program.
     */
    GLuint ID;
    Shader(const char *vertexCode, const char *fragmentCode);

    /**
     * @brief Activates this shader program for use in rendering.
     * (Calls glUseProgram)
     */
    void use();

    /**
     * @brief Sets a 4x4 matrix uniform variable in the shader.
     * @param name The name of the uniform (e.g., "projection").
     * @param mat The glm::mat4 matrix to send.
     */
    void setMat4(const std::string &name, const glm::mat4 &mat);

    /**
     * @brief Sets a 4-component vector uniform in the shader (e.g., for color).
     * @param name The name of the uniform (e.g., "drawColor").
     * @param v1 The first component (e.g., Red).
     * @param v2 The second component (e.g., Green).
     * @param v3 The third component (e.g., Blue).
     * @param v4 The fourth component (e.g., Alpha).
     */
    void setVec4(const std::string &name, float v1, float v2, float v3, float v4);

private:
    void compile(const char *vertexCode, const char *fragmentCode);
};

#endif