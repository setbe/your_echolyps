#pragma once

#include "device.hpp"

#include <vulkan/vulkan.h>
#include "../external/linmath.h"

namespace hi {
    // use `.init()`
    struct Model {
        struct Vertex {
            vec2 position;
            vec3 color;

            static void get_binding_description(VkVertexInputBindingDescription* out_binding) {
                out_binding[0].binding = 0;
                out_binding[0].stride = sizeof(Vertex);
                out_binding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            }

            static void get_attribute_descriptions(VkVertexInputAttributeDescription* out_attributes) {
                out_attributes[0].binding = 0;
                out_attributes[0].location = 0;
                out_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
                out_attributes[0].offset = offsetof(Vertex, position);

                out_attributes[1].binding = 0;
                out_attributes[1].location = 1;
                out_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                out_attributes[1].offset = offsetof(Vertex, color);
            }
        };

        inline explicit Model(EngineDevice& device) noexcept
            : device_{ device } {

        }

        inline Result init(const Vertex* array, uint32_t vertex_count) {
            Error error = Error::None;
            if ((error = create_vertex_buffers(array, vertex_count)) != Error::None)
                return { StageError::ModelInit, error };
            return { StageError::ModelInit, error };
        }


        inline ~Model() noexcept {
            vkDestroyBuffer(device_.device(), vertex_buffer_, nullptr);
            vkFreeMemory(device_.device(), vertex_buffer_memory_, nullptr);
        }

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;
        Model(Model&&) = delete;
        Model& operator=(Model&&) = delete;

        void bind(VkCommandBuffer command_buffer);
        void draw(VkCommandBuffer command_buffer);

    private:
        Error create_vertex_buffers(const Vertex* array, uint32_t array_size) noexcept;

        EngineDevice& device_;
        VkBuffer vertex_buffer_;
        VkDeviceMemory vertex_buffer_memory_;
        uint32_t vertex_count_;
    };
}