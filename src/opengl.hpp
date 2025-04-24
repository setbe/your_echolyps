#pragma once

#include "external/glad.hpp"
#include "external/linmath.h"

#include "higui/higui_platform.hpp"
#include "higui/higui_types.hpp"

#include "shaders.hpp"

#include <math.h>

namespace hi {
struct Opengl {
  private:
    unsigned vao_;
    unsigned buffer_;
    unsigned shader_program_;
    float time_value_;
    float green_value_;
    int vertex_color_location_;

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

    inline float get_time_value() const noexcept { return time_value_; }
    inline float get_green_value() const noexcept { return green_value_; }
    inline void update_green_value() noexcept {
        time_value_ = static_cast<float>(hi::time());
        green_value_ = (hi_sinf(time_value_) / 2.0f) + 0.5f;
    }
    inline int get_location_color() const noexcept {
        return vertex_color_location_;
    }

    unsigned get_shader_program() const noexcept { return shader_program_; }

    inline void draw() const noexcept {
        // glClearColor(0.1f, 0.2f, 0.3f, 0.0f); // use for non-black bg color
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    inline void framebuffer_resize(const ::hi::Callback &callback, int width,
                                   int height) const noexcept {
        glViewport(0, 0, width, height);
    }

  private:
    inline void bind_buffers() noexcept {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, buffer_);
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