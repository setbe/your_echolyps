#pragma once

#include "external/linmath.hpp"
#include "higui/higui_debug.hpp"

namespace hi {
struct Camera {
    math::vec3 position;
    math::vec3 front = {0.0f, 0.0f, -1.0f}; // direction front
    math::vec3 up = {0.0f, 1.0f, 0.0f};     // vector up
    math::vec3 right = {1.0f, 0.0f, 0.0f};  // vector right
    math::vec3 world_up = {0.0f, 1.0f, 0.0f};

    double yaw = -90.0; // horizontal
    double pitch = 0.0; // vertical

    float fov = 70;
    float movement_speed = 5.0f;
    float mouse_sensitivity = 0.1f;

    inline explicit Camera() noexcept {}

    inline void look_at(math::mat4x4 view) const noexcept {
        math::vec3 target{};
        math::vec3_add(target, position, front);
        math::mat4x4_look_at(view, position, target, up);
    }

    inline void process_mouse_movement(double xoffset,
                                       double yoffset) noexcept {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0)
            pitch = 89.0;
        if (pitch < -89.0)
            pitch = -89.0;

        update_vectors();
    }

    inline void move_forward(double delta_time) noexcept {
        math::vec3 velocity{};
        math::vec3_scale(velocity, front,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_add(position, position, velocity);
    }

    inline void move_backward(double delta_time) noexcept {
        math::vec3 velocity;
        math::vec3_scale(velocity, front,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_sub(position, position, velocity);
    }

    inline void move_right(double delta_time) noexcept {
        math::vec3 velocity;
        math::vec3_scale(velocity, right,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_add(position, position, velocity);
    }

    inline void move_left(double delta_time) noexcept {
        math::vec3 velocity;
        math::vec3_scale(velocity, right,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_sub(position, position, velocity);
    }

    inline void move_up(double delta_time) noexcept {
        math::vec3 velocity;
        math::vec3_scale(velocity, up,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_add(position, position, velocity);
    }

    inline void move_down(double delta_time) noexcept {
        math::vec3 velocity;
        math::vec3_scale(velocity, up,
                         movement_speed * static_cast<float>(delta_time));
        math::vec3_sub(position, position, velocity);
    }

  private:
    inline void update_vectors() noexcept {
        math::vec3 front_tmp;
        front_tmp[0] = static_cast<float>(math::cosf(math::radians(yaw)) *
                                          math::cosf(math::radians(pitch)));
        front_tmp[1] = static_cast<float>(math::sinf(math::radians(pitch)));
        front_tmp[2] = static_cast<float>(math::sinf(math::radians(yaw)) *
                                          math::cosf(math::radians(pitch)));
        math::vec3_norm(front, front_tmp);

        math::vec3_mul_cross(right, front, world_up);
        math::vec3_norm(right, right);
        math::vec3_mul_cross(up, right, front);
        math::vec3_norm(up, up);
    }
}; // struct Camera
} // namespace hi