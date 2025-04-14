#include "model.hpp"

#include <assert.h>

namespace hi {
    Error Model::create_vertex_buffers(const Vertex* array, uint32_t vertex_count) noexcept {
        vertex_count_ = vertex_count;
        assert(vertex_count_ >= 3 && "Vertex count must be at least 3");
        VkDeviceSize buffer_size = sizeof(array[0]) * vertex_count_;

        Error error;
        error = device_.create_buffer(
            buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertex_buffer_,
            vertex_buffer_memory_
            );
        if (error != Error::None)
            return error;

        void* data;
        vkMapMemory(device_.device(), vertex_buffer_memory_, 0, buffer_size, 0, &data);
        Vertex* dst = static_cast<Vertex*>(data);
        for (uint32_t i = 0; i < vertex_count_; ++i) {
            dst[i] = array[i];
        }
        vkUnmapMemory(device_.device(), vertex_buffer_memory_);

        return error;
    }

    void Model::bind(VkCommandBuffer command_buffer) {
        VkBuffer buffers[] = { vertex_buffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

    }
    void Model::draw(VkCommandBuffer command_buffer) {
        vkCmdDraw(command_buffer, vertex_count_, 1, 0, 0);
    }

} // namespace hi