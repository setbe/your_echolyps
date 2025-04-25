#pragma once

#include "external/glad.hpp"
#include "external/linmath.h"

#include "higui/higui_platform.hpp"
#include "higui/higui_types.hpp"

#include "fonts.hpp"
#include "shaders.hpp"

namespace hi {
struct Opengl {
    constexpr static unsigned vertex_count = 4;
    constexpr static float vertices[vertex_count * 8] = {
        // positions          // colors           // texture coords
        0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
        -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
    };
    constexpr static unsigned int indices[6] = {0, 1, 3, 1, 2, 3};

    explicit Opengl() noexcept;
    ~Opengl() noexcept {}

    Opengl(const Opengl &) = delete;
    Opengl &operator=(const Opengl &) = delete;
    Opengl(Opengl &&) = delete;
    Opengl &operator=(Opengl &&) = delete;

    inline void clear() const noexcept {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    inline void framebuffer_resize(const ::hi::Callback &callback, int width,
                                   int height) const noexcept {
        glViewport(0, 0, width, height);
    }
};

// helper function: guarantees error handling, returns the shader
unsigned compile_shader(unsigned type, const char *source) noexcept;

// guarantees error handling, returns the program
unsigned create_shader_program(const char *vert_source,
                               const char *frag_source) noexcept;
} // namespace hi
