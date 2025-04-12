#pragma once

#include "../higui/higui_types.hpp"
#include "device.hpp"

namespace hi {
    struct PipelineConfigInfo {
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
        VkPipelineRasterizationStateCreateInfo rasterization_info;
        VkPipelineMultisampleStateCreateInfo multisample_info;
        VkPipelineColorBlendAttachmentState color_blend_attachment;
        VkPipelineColorBlendStateCreateInfo color_blend_info;
        VkPipelineDepthStencilStateCreateInfo depth_stencil_info;
        VkPipelineLayout pipeline_layout = nullptr;
        VkRenderPass render_pass = nullptr;
        uint32_t subpass = 0;
    };

    // use `.init()`
    struct Pipeline {
        inline explicit Pipeline(EngineDevice& device_ref) noexcept
            : device_{ device_ref } {}

        inline ~Pipeline() noexcept {
            vkDestroyShaderModule(device_.device(), vert_shader_, nullptr);
            vkDestroyShaderModule(device_.device(), frag_shader_, nullptr);
            vkDestroyPipeline(device_.device(), graphics_pipeline_, nullptr);
        }

        Result init(const PipelineConfigInfo& config_info) noexcept;

        Pipeline() = delete;
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline(Pipeline&&) = delete;
        Pipeline& operator=(Pipeline&&) = delete;

        static PipelineConfigInfo default_config_info(uint32_t width, uint32_t hegiht) noexcept;
        
        Error create_pipeline_layout(VkPipelineLayout& pipeline_layout) noexcept;

        Error create_shader_module(const uint32_t* HI_RESTRICT code, unsigned int code_size,
            VkShaderModule* HI_RESTRICT shader_module) noexcept;

    private:
        EngineDevice& device_;
        VkPipeline graphics_pipeline_;
        VkShaderModule vert_shader_;
        VkShaderModule frag_shader_;
    };
} // namespace hi