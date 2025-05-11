#include "opengl.hpp"

namespace hi {
// helper function: guarantees error handling, returns the shader
unsigned compile_shader(unsigned type, const char *source) noexcept {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        ::hi::panic_notify(info_log);
    }
    return shader;
}

// guarantees error handling, returns the program
unsigned create_shader_program(const char *vert_source,
                               const char *frag_source) noexcept {
    unsigned program;
    unsigned vertex = compile_shader(GL_VERTEX_SHADER, vert_source);
    unsigned fragment = compile_shader(GL_FRAGMENT_SHADER, frag_source);

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        ::hi::panic_notify(info_log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glUseProgram(program);
    return program;
}

} // namespace hi