#pragma once

#include "../external/linmath.hpp"

namespace hi {
struct Camera {
    math::vec3 position;
    math::vec3 front = {0.0f, 0.0f, -1.0f}; // direction front
    math::vec3 up = {0.0f, 1.0f, 0.0f};     // vector up
    math::vec3 right = {1.0f, 0.0f, 0.0f};  // vector right
    constexpr static math::vec3 world_up = {0.0f, 1.0f, 0.0f};

    float yaw = -90.0f; // horizontal
    float pitch = 0.0f; // vertical

    float fov = 70.f;
    float movement_speed = 5.0f;
    float mouse_sensitivity = 0.1f;

    inline explicit Camera() noexcept {}

    inline void look_at(math::mat4x4 view) const noexcept {
        math::vec3 target{};
        math::vec3_add(target, position, front);
        math::mat4x4_look_at(view, position, target, up);
    }

    inline void process_mouse_movement(float xoffset, float yoffset) noexcept {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        pitch = pitch > 89.f ? 89.f : pitch;   // 89 is max
        pitch = pitch < -89.f ? -89.f : pitch; // -89 is min

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
        using namespace math;

        float pitch_rad = radians(pitch);
        float yaw_rad = radians(yaw);

        float cos_pitch = cosf(pitch_rad);

        vec3 front_tmp{
            cosf(yaw_rad) * cos_pitch, // x
            sinf(pitch_rad),           // y
            sinf(yaw_rad) * cos_pitch  // z
        };

        vec3_norm(front, front_tmp);

        vec3_mul_cross(right, front, world_up);
        vec3_norm(right, right);
        vec3_mul_cross(up, right, front);
        vec3_norm(up, up);
    }
}; // struct Camera
} // namespace hi