#include "./geometry_pass.h"

#include <stdlib.h>
#include <string.h>

#include "vk_utils.h"

void createGeometryRenderPass(
    VkDevice logicalDevice,
    VkFormat depthImageFormat,
    uint32_t colorAttachmentCount,
    VkFormat* colorAttachmentFormats,
    VkRenderPass* renderPass)
{
    /* ATTACHMENTS */
    uint32_t attachmentCount = colorAttachmentCount + 1;
    VkAttachmentDescription* attachments = malloc(
        attachmentCount * sizeof(VkAttachmentDescription));

    /* depth attachment */
    attachments[0].flags = 0;
    attachments[0].format = depthImageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAtttachmentRef;
    depthAtttachmentRef.attachment = 0;
    depthAtttachmentRef.layout 
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference* colorAttachmentRefs = malloc(
        attachmentCount * sizeof(VkAttachmentReference));
    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        attachments[i + 1].flags = 0;
        attachments[i + 1].format = colorAttachmentFormats[i];
        attachments[i + 1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i + 1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i + 1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i + 1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i + 1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i + 1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i + 1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        colorAttachmentRefs[i].attachment = i + 1;
        colorAttachmentRefs[i].layout 
            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    /* SUBPASS */
    VkSubpassDescription geometrySubpass;
    geometrySubpass.flags = 0;
    geometrySubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    geometrySubpass.inputAttachmentCount = 0;
    geometrySubpass.pInputAttachments = NULL;
    geometrySubpass.colorAttachmentCount = colorAttachmentCount;
    geometrySubpass.pColorAttachments = colorAttachmentRefs;
    geometrySubpass.pResolveAttachments = NULL;
    geometrySubpass.pDepthStencilAttachment = &depthAtttachmentRef;
    geometrySubpass.preserveAttachmentCount = 0;
    geometrySubpass.pPreserveAttachments = NULL;

    /* SUBPASS DEPENDENCIES */
    VkSubpassDependency geometrySubpassDependency;
    geometrySubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    geometrySubpassDependency.dstSubpass = 0;
    geometrySubpassDependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    geometrySubpassDependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    geometrySubpassDependency.srcAccessMask = 0;
    geometrySubpassDependency.dstAccessMask
        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    geometrySubpassDependency.dependencyFlags = 0;

    /* RENDER PASS */
    VkSubpassDescription subpasses[] = {
        geometrySubpass,
    };
    VkSubpassDependency subpassDependencies[] = {
        geometrySubpassDependency,
    };

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = NULL;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachmentCount;
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
        "creating geometry render pass");
}

void createObjectGeometryPipeline(
    VkDevice logicalDevice,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    uint32_t subpassIndex,
    VkExtent2D presentExtent,
    VkShaderModule vertShaderModule,
    VkShaderModule fragShaderModule,
    uint32_t colorAttachmentCount,
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
    viewport.width = (float)presentExtent.width;
    viewport.height = (float)presentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = presentExtent;

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
    rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
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

    /* DEPTH STENCIL */
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.sType
        = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = NULL;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    memset(&depthStencil.front, 0, sizeof(VkStencilOpState));
    memset(&depthStencil.back, 0, sizeof(VkStencilOpState));
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    /* COLOR BLENDING */
    VkPipelineColorBlendAttachmentState* blendAttachments
        = (VkPipelineColorBlendAttachmentState*)malloc(
            colorAttachmentCount * sizeof(VkPipelineColorBlendAttachmentState));
    for (
        uint32_t i = 0;
        i < colorAttachmentCount;
        i++) {
        blendAttachments[i].blendEnable = VK_FALSE;
        blendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachments[i].colorWriteMask
            = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    colorBlendInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.pNext = NULL;
    colorBlendInfo.flags = 0;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = colorAttachmentCount;
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
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
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
        "creating object geometry pipeline");
    free(blendAttachments);
}
