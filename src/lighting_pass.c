#include "./lighting_pass.h"

#include "vk_utils.h"

void createLightingRenderPass(
    VkDevice logicalDevice,
    VkFormat irradianceImageFormat,
    VkRenderPass* renderPass)
{
    /* ATTACHMENTS */
    VkAttachmentDescription irradianceAttachment;
    irradianceAttachment.flags = 0;
    irradianceAttachment.format = irradianceImageFormat;
    irradianceAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    irradianceAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    irradianceAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    irradianceAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    irradianceAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    irradianceAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    irradianceAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    /* SUBPASS */
    VkAttachmentReference irradianceAttachmentRef;
    irradianceAttachmentRef.attachment = 0;
    irradianceAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachments[] = {
        irradianceAttachmentRef,
    };

    VkSubpassDescription lightingSubpass;
    lightingSubpass.flags = 0;
    lightingSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    lightingSubpass.inputAttachmentCount = 0;
    lightingSubpass.pInputAttachments = NULL;
    lightingSubpass.colorAttachmentCount
        = sizeof(colorAttachments) / sizeof(colorAttachments[0]);
    lightingSubpass.pColorAttachments = colorAttachments;
    lightingSubpass.pResolveAttachments = NULL;
    lightingSubpass.pDepthStencilAttachment = NULL;
    lightingSubpass.preserveAttachmentCount = 0;
    lightingSubpass.pPreserveAttachments = NULL;

    /* SUBPASS DEPENDENCIES */
    VkSubpassDependency lightingSubpassDependency;
    lightingSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    lightingSubpassDependency.dstSubpass = 0;
    lightingSubpassDependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    lightingSubpassDependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    lightingSubpassDependency.srcAccessMask = 0;
    lightingSubpassDependency.dstAccessMask
        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    lightingSubpassDependency.dependencyFlags = 0;

    /* RENDER PASS */
    VkAttachmentDescription attachments[] = {
        irradianceAttachment,
    };
    VkSubpassDescription subpasses[] = {
        lightingSubpass,
    };
    VkSubpassDependency subpassDependencies[] = {
        lightingSubpassDependency,
    };

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = NULL;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount
        = sizeof(attachments) / sizeof(attachments[0]);
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount
        = sizeof(subpasses) / sizeof(subpasses[0]);
    renderPassCreateInfo.pSubpasses = subpasses;
    renderPassCreateInfo.dependencyCount
        = sizeof(subpassDependencies) / sizeof(subpassDependencies[0]);
    renderPassCreateInfo.pDependencies = subpassDependencies;

    handleVkResult(
        vkCreateRenderPass(
            logicalDevice,
            &renderPassCreateInfo,
            NULL,
            renderPass),
        "creating lighting render pass");
}

void createDenoiseRenderPass(
    VkDevice logicalDevice,
    VkFormat swapImageFormat,
    VkRenderPass* renderPass)
{
    /* ATTACHMENTS */
    VkAttachmentDescription swapAttachment;
    swapAttachment.flags = 0;
    swapAttachment.format = swapImageFormat;
    swapAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    swapAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swapAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swapAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    swapAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    swapAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    /* SUBPASS */
    VkAttachmentReference swapAttachmentRef;
    swapAttachmentRef.attachment = 0;
    swapAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachments[] = {
        swapAttachmentRef,
    };

    VkSubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount
        = sizeof(colorAttachments) / sizeof(colorAttachments[0]);
    subpass.pColorAttachments = colorAttachments;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = NULL;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    /* SUBPASS DEPENDENCIES */
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask
        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;

    /* RENDER PASS */
    VkAttachmentDescription attachments[] = {
        swapAttachment,
    };
    VkSubpassDescription subpasses[] = {
        subpass,
    };
    VkSubpassDependency subpassDependencies[] = {
        subpassDependency,
    };

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = NULL;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount
        = sizeof(attachments) / sizeof(attachments[0]);
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount
        = sizeof(subpasses) / sizeof(subpasses[0]);
    renderPassCreateInfo.pSubpasses = subpasses;
    renderPassCreateInfo.dependencyCount
        = sizeof(subpassDependencies) / sizeof(subpassDependencies[0]);
    renderPassCreateInfo.pDependencies = subpassDependencies;

    handleVkResult(
        vkCreateRenderPass(
            logicalDevice,
            &renderPassCreateInfo,
            NULL,
            renderPass),
        "creating denoise render pass");
}

void createFullScreenFragPipeline(
    VkDevice logicalDevice,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    uint32_t subpassIndex,
    VkExtent2D extent,
    VkShaderModule vertShaderModule,
    VkShaderModule fragShaderModule,
    VkPipeline* pipeline)
{
    /* SHADER STAGES */
    VkPipelineShaderStageCreateInfo vertShaderStage;
    vertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStage.pNext = NULL;
    vertShaderStage.flags = 0;
    vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStage.module = vertShaderModule;
    vertShaderStage.pName = "main";
    vertShaderStage.pSpecializationInfo = NULL;
    VkPipelineShaderStageCreateInfo fragShaderStage;
    fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStage.pNext = NULL;
    fragShaderStage.flags = 0;
    fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStage.module = fragShaderModule;
    fragShaderStage.pName = "main";
    fragShaderStage.pSpecializationInfo = NULL;

    /* VERTEX INPUT */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = NULL;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = NULL;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL;

    /* INPUT ASSEMBLY */
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.pNext = NULL;
    inputAssemblyInfo.flags = 0;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    /* VIEWPORT */
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportInfo;
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.pNext = NULL;
    viewportInfo.flags = 0;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    /* RASTERIZATION */
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    rasterizationInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.pNext = NULL;
    rasterizationInfo.flags = 0;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationInfo.lineWidth = 1.0f;

    /* MULTISAMPLING */
    VkPipelineMultisampleStateCreateInfo multisamplingInfo;
    multisamplingInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.pNext = NULL;
    multisamplingInfo.flags = 0;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.minSampleShading = 0.0f;
    multisamplingInfo.pSampleMask = NULL;
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingInfo.alphaToOneEnable = VK_FALSE;

    /* COLOR BLENDING */
    VkPipelineColorBlendAttachmentState blend;
    blend.blendEnable = VK_FALSE;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend.colorBlendOp = VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend.alphaBlendOp = VK_BLEND_OP_ADD;
    blend.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendAttachmentState blendAttachments[] = {
        blend,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    colorBlendInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.pNext = NULL;
    colorBlendInfo.flags = 0;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount
        = sizeof(blendAttachments) / sizeof(blendAttachments[0]);
    colorBlendInfo.pAttachments = blendAttachments;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    /* GRAPHICS PIPELINE */
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = NULL;
    pipelineCreateInfo.flags = 0;
    VkPipelineShaderStageCreateInfo shaderStages[]
        = { vertShaderStage, fragShaderStage };
    pipelineCreateInfo.stageCount
        = sizeof(shaderStages) / sizeof(shaderStages[0]);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pTessellationState = NULL;
    pipelineCreateInfo.pViewportState = &viewportInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingInfo;
    pipelineCreateInfo.pDepthStencilState = NULL;
    pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
    pipelineCreateInfo.pDynamicState = NULL;
    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = subpassIndex;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;

    handleVkResult(
        vkCreateGraphicsPipelines(
            logicalDevice,
            VK_NULL_HANDLE,
            1,
            &pipelineCreateInfo,
            NULL,
            pipeline),
        "creating lighting pipeline");
}

