#include "pipeline.hpp"

#include <assert.h>

#include "../shaders.hpp"

namespace hi {
    PipelineConfigInfo Pipeline::default_config_info(uint32_t width, uint32_t height) noexcept {
        PipelineConfigInfo config_info{};
        config_info.viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        config_info.scissor = {
            .offset = {0, 0},
            .extent = {width, height}
        };

        config_info.input_assembly_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        config_info.rasterization_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,  // Optional
            .depthBiasClamp = 0.0f,           // Optional
            .depthBiasSlopeFactor = 0.0f,     // Optional
            .lineWidth = 1.0f,
        };

        config_info.multisample_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,           // Optional
            .pSampleMask = nullptr,             // Optional
            .alphaToCoverageEnable = VK_FALSE,  // Optional
            .alphaToOneEnable = VK_FALSE,       // Optional
        };

        config_info.color_blend_attachment = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,   // Optional
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,  // Optional
            .colorBlendOp = VK_BLEND_OP_ADD,              // Optional
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,   // Optional
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,  // Optional
            .alphaBlendOp = VK_BLEND_OP_ADD,              // Optional
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT,
        };

        config_info.color_blend_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,  // Optional
            .attachmentCount = 1,
            .pAttachments = &config_info.color_blend_attachment,
            .blendConstants = { 0, 0, 0, 0 } // Optional
        };

        config_info.depth_stencil_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},  // Optional
            .back = {},   // Optional
            .minDepthBounds = 0.0f,  // Optional
            .maxDepthBounds = 1.0f,  // Optional
        };
        return config_info;
    }

    Error Pipeline::create_shader_module(
        const uint32_t* HI_RESTRICT code, unsigned int code_size, VkShaderModule* HI_RESTRICT shader_module) noexcept {
        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code_size,
            .pCode = code,
        };
        if (vkCreateShaderModule(device_.device(), &create_info, nullptr, shader_module) != VK_SUCCESS)
            return hi::Error::CreateShaderModule;
        return hi::Error::None;
    }

    Result Pipeline::init(const PipelineConfigInfo& config_info) noexcept {
        assert(config_info.pipeline_layout != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: No `.pipeline_layout` provided in config_info");
        assert(config_info.render_pass != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: No `.render_pass` provided in config_info");
        StageError stage_error = StageError::CreateShaderModule;
        Error error_code = Error::None;

        { // Shader compiling
            if ((error_code = create_shader_module(
                default_vert_spv,
                default_vert_spv_len * sizeof(default_vert_spv_len),
                &vert_shader_)) != Error::None)
                // `Vertex` shader failed to compile
                return { stage_error, error_code };

            if ((error_code = create_shader_module(
                default_frag_spv,
                default_frag_spv_len * sizeof(default_frag_spv_len),
                &frag_shader_)) != Error::None)
                // `Fragment` shader failed to compile
                return { stage_error, error_code };
        }

        VkPipelineShaderStageCreateInfo shader_stages[2];
        shader_stages[0] = { // Vertex
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_shader_,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };

        shader_stages[1] = { // Fragment
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_shader_,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            // Binding
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            // Attribute
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        VkPipelineViewportStateCreateInfo viewport_info {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &config_info.viewport,
            .scissorCount = 1,
            .pScissors = &config_info.scissor
        };

        VkGraphicsPipelineCreateInfo pipeline_info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shader_stages,
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &config_info.input_assembly_info,
            .pViewportState = &viewport_info,
            .pRasterizationState = &config_info.rasterization_info,
            .pMultisampleState = &config_info.multisample_info,
            .pDepthStencilState = &config_info.depth_stencil_info,
            .pColorBlendState = &config_info.color_blend_info,
            .pDynamicState = nullptr,

            .layout = config_info.pipeline_layout,
            .renderPass = config_info.render_pass,
            .subpass = config_info.subpass,

            .basePipelineHandle = VK_NULL_HANDLE, // handle = Optimization
            .basePipelineIndex = -1,
        };

        stage_error = StageError::CreatePipeline;
        if (vkCreateGraphicsPipelines(device_.device(), // cache = Optimization
            VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
            return { stage_error, hi::Error::GraphicsPipeline }; // Couldn't create graphics pipeline
        return { stage_error, error_code };

    }

    Error Pipeline::create_pipeline_layout(VkPipelineLayout& pipeline_layout) noexcept {
        VkPipelineLayoutCreateInfo layout_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };
        if (vkCreatePipelineLayout(device_.device(), &layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
            return Error::CreatePipelineLayout;
        }
        return Error::None;
    }

    void Pipeline::bind(VkCommandBuffer command_buffer) noexcept {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
    }

} // namespace hi