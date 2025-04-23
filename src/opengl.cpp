#include "opengl.hpp"

namespace hi {
// guarantees error handling, returns the program
static inline unsigned create_shader_program(const char *vert_source,
                                             const char *frag_source) noexcept;
Opengl::Opengl() noexcept {
#if !defined(HI_MULTITHREADING_USED)
    static bool first_time = true;
#else  // use multithreading
    thread_local bool first_time = true;
#endif // HI_MULTITHREADING_USED

    if (first_time) {
        window::load_gl();
        first_time = false;
    }
    this->shader_program = create_shader_program(default_vert, default_frag);
    this->bind_buffers();
}

Opengl::~Opengl() noexcept {
    if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
    }
    if (buffer != 0) {
        glDeleteBuffers(1, &buffer);
        buffer = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

// helper function: guarantees error handling, returns the shader
static inline unsigned compile_shader(unsigned type,
                                      const char *source) noexcept {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        ::hi::panic_notify(::hi::Error::CompileShader, info_log);
    }
    return shader;
}

// guarantees error handling, returns the program
static inline unsigned create_shader_program(const char *vert_source,
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
        ::hi::panic_notify(::hi::Error::CompileShader, info_log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glUseProgram(program);
    return program;
}

} // namespace hi