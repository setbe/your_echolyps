#pragma once

#include "external/glad.hpp"

#include "higui/higui_platform.hpp"
#include "higui/higui_types.hpp"

#include "shaders.hpp"

namespace hi {
struct Opengl {
  private:
    unsigned vao;
    unsigned buffer;
    unsigned shader_program;

  public:
    constexpr static unsigned vertex_count = 3;
    constexpr static float vertices[vertex_count * 5] = {
        // x, y,     r, g, b
        -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // red
        0.0f,  0.5f,  0.0f, 1.0f, 0.0f, // green
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f  // blue
    };

    explicit Opengl() noexcept;
    ~Opengl() noexcept;

    Opengl(const Opengl &) = delete;
    Opengl &operator=(const Opengl &) = delete;
    Opengl(Opengl &&) = delete;
    Opengl &operator=(Opengl &&) = delete;

    inline void draw() const noexcept {
        // glClearColor(0.1f, 0.2f, 0.3f, 0.0f); // use for non-black bg color
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    inline void framebuffer_resize(const ::hi::Callback &callback, int width,
                                   int height) const noexcept {
        glViewport(0, 0, width, height);
    }

  private:
    inline void bind_buffers() noexcept {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);

        // position: location = 0, 2 floats
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)0);

        // color: location = 1, 3 floats
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(2 * sizeof(float)));

        // glBindVertexArray(0);
    }
}; // struct Opengl
} // namespace hi