#pragma once

#include "external/glad.hpp"
#include "external/linmath.hpp"

#include "higui/higui_platform.hpp"
#include "higui/higui_types.hpp"

#include "fonts.hpp"
#include "shaders.hpp"

namespace hi {
// returns the program, guarantees error handling
unsigned create_shader_program(const char *vert_source,
                               const char *frag_source) noexcept;

struct Opengl {
    struct VAO {
      private:
        unsigned id;

      public:
        inline explicit VAO() noexcept {
            glGenVertexArrays(1, &id);
            if (!id)
                panic(Result{Stage::Opengl, Error::GenerateVertexArray});
        }
        inline ~VAO() noexcept { glDeleteVertexArrays(1, &id); }

        VAO(const VAO &) = delete;
        VAO &operator=(const VAO &) = delete;
        VAO(VAO &&) = delete;
        VAO &operator=(VAO &&) = delete;

        inline unsigned get() const noexcept { return id; }
        inline void bind() const noexcept { glBindVertexArray(id); }
        static inline void unbind() noexcept { glBindVertexArray(0); }
    }; // struct VAO

    struct VBO {
      private:
        unsigned id;

      public:
        inline explicit VBO() noexcept {
            glGenBuffers(1, &id);
            if (!id)
                panic(Result{Stage::Opengl, Error::GenerateBuffer});
        }
        inline ~VBO() noexcept { glDeleteBuffers(1, &id); }

        VBO(const VBO &) = delete;
        VBO &operator=(const VBO &) = delete;
        VBO(VBO &&) = delete;
        VBO &operator=(VBO &&) = delete;

        inline unsigned get() const noexcept { return id; }

        inline void buffer_data(unsigned target, unsigned size,
                                const void *data,
                                unsigned usage) const noexcept {
            glBufferData(target, size, data, usage);
        }
        inline void buffer_base(unsigned target,
                                unsigned index) const noexcept {
            glBindBufferBase(target, index, id);
        }
        inline void sub_data(unsigned target, GLintptr offset, unsigned size,
                             const void *data) const noexcept {
            glBufferSubData(target, offset, size, data);
        }

        inline void bind(unsigned target) const noexcept {
            glBindBuffer(target, id);
        }
        static inline void unbind(unsigned target) noexcept {
            glBindBuffer(target, 0);
        }
    }; // struct VBO

    using EBO = VBO;
    using SSBO = VBO;

    struct ShaderProgram {
      private:
        unsigned id;

      public:
        inline explicit ShaderProgram(const char *vertex_source,
                                      const char *fragment_source) noexcept
            : id(create_shader_program(vertex_source, fragment_source)) {}
        inline ~ShaderProgram() noexcept { glDeleteProgram(id); }

        ShaderProgram(const ShaderProgram &) = delete;
        ShaderProgram &operator=(const ShaderProgram &) = delete;
        ShaderProgram(ShaderProgram &&) = delete;
        ShaderProgram &operator=(ShaderProgram &&) = delete;

        inline void use() const noexcept { glUseProgram(id); }
        inline unsigned get() const noexcept { return id; }
    }; // struct ShaderProgram

    struct Texture {
      private:
        unsigned id;

      public:
        Texture() noexcept {
            glGenTextures(1, &id);
            if (!id)
                panic(Result{Stage::Opengl, Error::GenerateTexture});
        }

        ~Texture() noexcept { glDeleteTextures(1, &id); }

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;
        Texture(Texture &&) = delete;
        Texture &operator=(Texture &&) = delete;

        unsigned get() const noexcept { return id; }
        void bind(unsigned target) const noexcept { glBindTexture(target, id); }
        static void unbind(unsigned target) noexcept {
            glBindTexture(target, 0);
        }
    }; // struct Texture

    explicit Opengl() noexcept {
        static bool first_time = true;
        if (first_time) {
            window::load_gl();
            first_time = false;
        }
        glEnable(GL_CULL_FACE);
        // glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    }
    ~Opengl() noexcept {}

    Opengl(const Opengl &) = delete;
    Opengl &operator=(const Opengl &) = delete;
    Opengl(Opengl &&) = delete;
    Opengl &operator=(Opengl &&) = delete;

    inline void clear() const noexcept { glClear(GL_COLOR_BUFFER_BIT); }

    inline void framebuffer_resize(const ::hi::Callback &callback, int width,
                                   int height) const noexcept {
        glViewport(0, 0, width, height);
    }
};
} // namespace hi
