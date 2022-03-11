#include "./renderer.h"

#include <stdlib.h>
#include <string.h>

#include <cglm/cglm.h>

#include "utils.h"
#include "vk_utils.h"

static void createSwapchain(
    VkDevice logicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surfaceFormat,
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    VkPresentModeKHR presentMode,
    uint32_t graphicsFamilyIndex,
    uint32_t presentFamilyIndex,
    VkExtent2D extent,
    VkSwapchainKHR* swapchain)
{
    uint32_t minSwapLen = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0)
        minSwapLen = MIN(
            minSwapLen,
            surfaceCapabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = NULL;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = minSwapLen;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphicsFamilyIndex != presentFamilyIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        uint32_t queueFamilyIndices[] = {
            graphicsFamilyIndex,
            presentFamilyIndex
        };
        swapchainCreateInfo.queueFamilyIndexCount
            = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = NULL;
    }
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    handleVkResult(
        vkCreateSwapchainKHR(
            logicalDevice,
            &swapchainCreateInfo,
            NULL,
            swapchain),
        "creating swapchain");
}

static void createDepthImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkFormat depthImageFormat,
    VkExtent2D extent,
    VkImage* depthImage,
    VkDeviceMemory* depthImageMemory,
    VkImageView* depthImageView)
{
    VkExtent3D extent3D;
    extent3D.width = extent.width;
    extent3D.height = extent.height;
    extent3D.depth = 1;

    createImage(
        logicalDevice,
        physicalDevice,
        VK_IMAGE_TYPE_2D,
        depthImageFormat,
        extent3D,
        0,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        false,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = *depthImage;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = depthImageFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &viewCreateInfo,
            NULL,
            depthImageView),
        "creating depth image view");
}

static void createObjRenderPass(
    VkDevice logicalDevice,
    VkFormat swapImageFormat,
    VkFormat depthImageFormat,
    VkRenderPass* renderPass)
{
    /* RENDER PASS ATTACHMENTS */
    VkAttachmentDescription colorAttachment;
    colorAttachment.flags = 0;
    colorAttachment.format = swapImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment;
    depthAttachment.flags = 0;
    depthAttachment.format = depthImageFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAtttachmentRef;
    depthAtttachmentRef.attachment = 1;
    depthAtttachmentRef.layout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* OBJ SUBPASS */
    VkSubpassDescription objSubpass;
    objSubpass.flags = 0;
    objSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    objSubpass.inputAttachmentCount = 0;
    objSubpass.pInputAttachments = NULL;
    objSubpass.colorAttachmentCount = 1;
    objSubpass.pColorAttachments = &colorAttachmentRef;
    objSubpass.pResolveAttachments = NULL;
    objSubpass.pDepthStencilAttachment = &depthAtttachmentRef;
    objSubpass.preserveAttachmentCount = 0;
    objSubpass.pPreserveAttachments = NULL;

    /* SUBPASS DEPENDENCIES */
    VkSubpassDependency objSubpassDependency;
    objSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    objSubpassDependency.dstSubpass = 0;
    objSubpassDependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    objSubpassDependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    objSubpassDependency.srcAccessMask = 0;
    objSubpassDependency.dstAccessMask
        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    objSubpassDependency.dependencyFlags = 0;

    /* RENDER PASS */
    VkAttachmentDescription attachments[] = {
        colorAttachment,
        depthAttachment
    };
    VkSubpassDescription subpasses[] = {
        objSubpass
    };
    VkSubpassDependency subpassDependencies[] = {
        objSubpassDependency,
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
        "creating object render pass");
}

static void createPipeline(
    VkDevice logicalDevice,
    VkPipelineLayout layout,
    VkRenderPass objRenderPass,
    VkExtent2D presentExtent,
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
    getObjectBackFaceVertexInfo(
        &vertexInputInfo.vertexBindingDescriptionCount,
        &vertexInputInfo.pVertexBindingDescriptions,
        &vertexInputInfo.vertexAttributeDescriptionCount,
        &vertexInputInfo.pVertexAttributeDescriptions);

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
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationInfo.lineWidth = 1.0f;

    /* MULTISAMPLING */
    VkPipelineMultisampleStateCreateInfo multisamplingInfo;
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
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
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
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
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.pNext = NULL;
    colorBlendInfo.flags = 0;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    /* GRAPHICS PIPELINE */
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = NULL;
    pipelineCreateInfo.flags = 0;
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStage, fragShaderStage };
    pipelineCreateInfo.stageCount = sizeof(shaderStages) / sizeof(shaderStages[0]);
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
    pipelineCreateInfo.renderPass = objRenderPass;
    pipelineCreateInfo.subpass = 0;
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
        "creating graphics pipeline");
}

static void createFramebuffers(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    VkExtent2D extent,
    uint32_t count,
    VkImageView* swapImageViews,
    VkImageView depthImageView,
    VkFramebuffer* framebuffers)
{
    for (uint32_t i = 0; i < count; i++) {
        VkImageView attachments[] = { swapImageViews[i], depthImageView };

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = NULL;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount
            = sizeof(attachments) / sizeof(attachments[0]);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = 1;

        handleVkResult(
            vkCreateFramebuffer(
                logicalDevice,
                &framebufferCreateInfo,
                NULL,
                &framebuffers[i]),
            "creating framebuffer");
    }
}

static void createRenderCommandBuffers(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    VkExtent2D presentExtent,
    VkPipeline objPipeline,
    VkPipelineLayout objPipelineLayout,
    VkDescriptorSet* renderDescriptorSets,
    ObjectStorage* objStorage,
    VkCommandPool pool,
    uint32_t swapLen,
    VkFramebuffer* framebuffers,
    VkCommandBuffer* commandBuffers)
{
    /* ALLOCATE COMMAND BUFFERS */
    {
        VkCommandBufferAllocateInfo allocInfo;
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = NULL;
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = swapLen;

        handleVkResult(
            vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers),
            "allocating render command buffers");
    }

    for (int s = 0; s < swapLen; s++) {
        /* BEGIN COMMAND BUFFER */
        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = NULL;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = NULL;
        handleVkResult(
            vkBeginCommandBuffer(commandBuffers[s], &beginInfo),
            "begin recording render command buffers");

        VkClearValue clearValues[2];
        memset(clearValues, 0, sizeof(clearValues));
        clearValues[0].color.float32[0] = 128.0f / 255.0;
        clearValues[0].color.float32[1] = 218.0f / 255.0;
        clearValues[0].color.float32[2] = 251.0f / 255.0;
        clearValues[0].color.float32[3] = 1.0f;

        clearValues[1].depthStencil.depth = 1.0f;

        VkRenderPassBeginInfo renderPassInfo;
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.pNext = NULL;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[s];
        renderPassInfo.renderArea.offset = (VkOffset2D) { 0, 0 };
        renderPassInfo.renderArea.extent = presentExtent;
        renderPassInfo.clearValueCount
            = sizeof(clearValues) / sizeof(clearValues[0]);
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(
            commandBuffers[s],
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
            commandBuffers[s],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            objPipeline);

        vkCmdBindDescriptorSets(
            commandBuffers[s],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            objPipelineLayout,
            0,
            1,
            &renderDescriptorSets[s],
            0,
            NULL);

        VkDeviceSize voxTriVertexBufferOffsets[] = { 0 };
        vkCmdBindVertexBuffers(
            commandBuffers[s],
            0,
            1,
            &objStorage->backFaceVertBuffer,
            voxTriVertexBufferOffsets);

        for (int o = 0; o < objStorage->filled; o++) {
            vkCmdDraw(
                commandBuffers[s],
                MAX_OBJ_BACK_FACE_VERT_COUNT,
                1,
                objStorage->backFaceVertBufferOffsets[o],
                0);
        }

        /* END COMAND BUFFER */
        vkCmdEndRenderPass(commandBuffers[s]);

        handleVkResult(
            vkEndCommandBuffer(commandBuffers[s]),
            "recording render command buffer");
    }
}

void Renderer_init(
    Renderer* renderer,
    VulkanDevice* device,
    VkExtent2D presentExtent,
    ObjectStorage* objStorage)
{
    renderer->presentExtent = presentExtent;

    /* GBUFFER */
    renderer->swapImageFormat = device->physicalProperties.surfaceFormat.format;
    createSwapchain(
        device->logical,
        device->surface,
        device->physicalProperties.surfaceFormat,
        device->physicalProperties.surfaceCapabilities,
        device->physicalProperties.presentMode,
        device->physicalProperties.graphicsFamilyIndex,
        device->physicalProperties.presentFamilyIndex,
        renderer->presentExtent,
        &renderer->swapchain);
    handleVkResult(
        vkGetSwapchainImagesKHR(
            device->logical,
            renderer->swapchain,
            &renderer->swapLen,
            NULL),
        "getting swapchain image count");
    renderer->swapImages
        = (VkImage*)malloc(renderer->swapLen * sizeof(VkImage));
    handleVkResult(
        vkGetSwapchainImagesKHR(
            device->logical,
            renderer->swapchain,
            &renderer->swapLen,
            renderer->swapImages),
        "getting swapchain images");
    renderer->swapImageViews
        = (VkImageView*)malloc(renderer->swapLen * sizeof(VkImageView));
    for (int i = 0; i < renderer->swapLen; i++) {
        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = NULL;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = renderer->swapImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = renderer->swapImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange = subresourceRange;
        handleVkResult(
            vkCreateImageView(
                device->logical,
                &imageViewCreateInfo,
                NULL,
                &renderer->swapImageViews[i]),
            "creating swapchain image view");
    }

    renderer->depthImageFormat = device->physicalProperties.depthImageFormat;
    createDepthImage(
        device->logical,
        device->physical,
        renderer->depthImageFormat,
        renderer->presentExtent,
        &renderer->depthImage,
        &renderer->depthImageMemory,
        &renderer->depthImageView);

    /* DESCRIPTOR POOL */
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = renderer->swapLen;

        VkDescriptorPoolCreateInfo poolCreateInfo;
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = NULL;
        poolCreateInfo.flags = 0;
        poolCreateInfo.maxSets = renderer->swapLen;
        poolCreateInfo.poolSizeCount = 1;
        poolCreateInfo.pPoolSizes = &poolSize;

        handleVkResult(
            vkCreateDescriptorPool(
                device->logical,
                &poolCreateInfo,
                NULL,
                &renderer->descriptorPool),
            "creating renderer descriptor pool");
    }

    /* RENDER DESCRIPTOR SET LAYOUT */
    {
        VkDescriptorSetLayoutBinding uboBinding;
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.sType
            = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = NULL;
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.bindingCount = 1;
        layoutCreateInfo.pBindings = &uboBinding;

        handleVkResult(
            vkCreateDescriptorSetLayout(
                device->logical,
                &layoutCreateInfo,
                NULL,
                &renderer->renderDescriptorSetLayout),
            "creating render descriptor set layout");
    }

    /* RENDER DESCRIPTOR SET BUFFER */
    createBuffer(
        device->logical,
        device->physical,
        renderer->swapLen * sizeof(RenderDescriptorData),
        0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer->renderDescriptorBuffer,
        &renderer->renderDescriptorBufferMemory);

    /* RENDER DESCRIPTOR SETS */
    renderer->renderDescriptorSets = (VkDescriptorSet*)malloc(
        renderer->swapLen * sizeof(VkDescriptorSet));
    allocateDescriptorSets(
        device->logical,
        renderer->renderDescriptorSetLayout,
        renderer->descriptorPool,
        renderer->swapLen,
        renderer->renderDescriptorSets);
    for (uint32_t i = 0; i < renderer->swapLen; i++) {
        VkDescriptorBufferInfo bufInfo;
        bufInfo.buffer = renderer->renderDescriptorBuffer;
        bufInfo.offset = i * sizeof(RenderDescriptorData);
        bufInfo.range = sizeof(RenderDescriptorData);

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = renderer->renderDescriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pImageInfo = NULL;
        write.pBufferInfo = &bufInfo;
        write.pTexelBufferView = NULL;
        vkUpdateDescriptorSets(device->logical, 1, &write, 0, NULL);
    }

    /* RENDER PASS */
    createObjRenderPass(
        device->logical,
        renderer->swapImageFormat,
        renderer->depthImageFormat,
        &renderer->objRenderPass);

    /* PIPELINE LAYOUT */
    {
        VkDescriptorSetLayout pipelineDescriptorSetLayouts[] = {
            renderer->renderDescriptorSetLayout
        };

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType
            = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = NULL;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount
            = sizeof(pipelineDescriptorSetLayouts)
            / sizeof(pipelineDescriptorSetLayouts[0]);
        pipelineLayoutCreateInfo.pSetLayouts = pipelineDescriptorSetLayouts;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

        handleVkResult(
            vkCreatePipelineLayout(
                device->logical,
                &pipelineLayoutCreateInfo,
                NULL,
                &renderer->pipelineLayout),
            "creating pipeline layout");
    }

    /* PIPELINE */
    {
        VkShaderModule objVertShaderModule, objFragShaderModule;
        createShaderModule(
            device->logical,
            "obj.vert.spv",
            &objVertShaderModule);
        createShaderModule(
            device->logical,
            "obj.frag.spv",
            &objFragShaderModule);
        createPipeline(
            device->logical,
            renderer->pipelineLayout,
            renderer->objRenderPass,
            renderer->presentExtent,
            objVertShaderModule,
            objFragShaderModule,
            &renderer->pipeline);
        vkDestroyShaderModule(
            device->logical,
            objVertShaderModule,
            NULL);
        vkDestroyShaderModule(
            device->logical,
            objFragShaderModule,
            NULL);
    }

    /* FRAMEBUFFERS */
    renderer->framebuffers
        = (VkFramebuffer*)malloc(renderer->swapLen * sizeof(VkFramebuffer));
    createFramebuffers(
        device->logical,
        renderer->objRenderPass,
        renderer->presentExtent,
        renderer->swapLen,
        renderer->swapImageViews,
        renderer->depthImageView,
        renderer->framebuffers);

    /* COMMAND BUFFERS */
    renderer->commandBuffers
        = (VkCommandBuffer*)malloc(renderer->swapLen * sizeof(VkCommandBuffer));
    createRenderCommandBuffers(
        device->logical,
        renderer->objRenderPass,
        renderer->presentExtent,
        renderer->pipeline,
        renderer->pipelineLayout,
        renderer->renderDescriptorSets,
        objStorage,
        device->commandPool,
        renderer->swapLen,
        renderer->framebuffers,
        renderer->commandBuffers);

    /* SYNCHRONIZATION */
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = NULL;
        semaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo fenceCreateInfo;
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = NULL;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            handleVkResult(
                vkCreateSemaphore(
                    device->logical,
                    &semaphoreCreateInfo,
                    NULL,
                    &renderer->imageAvailableSemaphores[i]),
                "creating image available semaphore");
            handleVkResult(
                vkCreateSemaphore(
                    device->logical,
                    &semaphoreCreateInfo,
                    NULL,
                    &renderer->renderFinishedSemaphores[i]),
                "creating render finished semaphore");
            handleVkResult(
                vkCreateFence(
                    device->logical,
                    &fenceCreateInfo,
                    NULL,
                    &renderer->renderFinishedFences[i]),
                "creating render finished fence");
        }
    }

    renderer->swapchainImageFences
        = (VkFence**)malloc(renderer->swapLen * sizeof(VkFence*));
    memset(renderer->swapchainImageFences, 0, renderer->swapLen * sizeof(VkFence*));

    renderer->currentFrame = 0;
}

void Renderer_drawFrame(
    Renderer* renderer,
    VulkanDevice* device,
    mat4 view,
    mat4 proj)
{
    handleVkResult(
        vkWaitForFences(
            device->logical,
            1,
            &renderer->renderFinishedFences[renderer->currentFrame],
            VK_TRUE,
            UINT64_MAX),
        "waiting for render finished fence");

    uint32_t imageIndex;
    handleVkResult(
        vkAcquireNextImageKHR(
            device->logical,
            renderer->swapchain,
            UINT64_MAX,
            renderer->imageAvailableSemaphores[renderer->currentFrame],
            VK_NULL_HANDLE,
            &imageIndex),
        "acquiring next swapchain image");

    if (renderer->swapchainImageFences[imageIndex] != NULL)
        handleVkResult(
            vkWaitForFences(
                device->logical,
                1,
                renderer->swapchainImageFences[imageIndex],
                VK_TRUE,
                UINT64_MAX),
            "waiting for swapchain image render finished fence");

    vkResetFences(
        device->logical,
        1,
        &renderer->renderFinishedFences[renderer->currentFrame]);
    for (int i = 0; i < renderer->swapLen; i++)
        if (renderer->swapchainImageFences[i]
            == &renderer->renderFinishedFences[renderer->currentFrame]) {
            renderer->swapchainImageFences[i] = NULL;
            break;
        }
    renderer->swapchainImageFences[imageIndex]
        = &renderer->renderFinishedFences[renderer->currentFrame];

    RenderDescriptorData* renderData;
    vkMapMemory(
        device->logical,
        renderer->renderDescriptorBufferMemory,
        imageIndex * sizeof(RenderDescriptorData),
        sizeof(RenderDescriptorData),
        0,
        (void**)&renderData);

    glm_mat4_copy(view, renderData->view);
    glm_mat4_copy(proj, renderData->proj);

    vkUnmapMemory(device->logical, renderer->renderDescriptorBufferMemory);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores
        = &renderer->imageAvailableSemaphores[renderer->currentFrame];
    VkPipelineStageFlags waitStage
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores
        = &renderer->renderFinishedSemaphores[renderer->currentFrame];

    handleVkResult(
        vkQueueSubmit(
            device->graphicsQueue,
            1,
            &submitInfo,
            renderer->renderFinishedFences[renderer->currentFrame]),
        "submitting render command buffer");

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores
        = &renderer->renderFinishedSemaphores[renderer->currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &renderer->swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;

    handleVkResult(
        vkQueuePresentKHR(device->presentQueue, &presentInfo),
        "submitting present call");

    renderer->currentFrame
        = (renderer->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer_destroy(
    Renderer* renderer,
    VkDevice logicalDevice)
{
    /* IMAGES AND FRAMEBUFFERS */
    for (uint32_t i = 0; i < renderer->swapLen; i++) {
        vkDestroyImageView(logicalDevice, renderer->swapImageViews[i], NULL);
        vkDestroyFramebuffer(logicalDevice, renderer->framebuffers[i], NULL);
    }
    vkDestroySwapchainKHR(logicalDevice, renderer->swapchain, NULL);
    vkDestroyImage(logicalDevice, renderer->depthImage, NULL);
    vkFreeMemory(logicalDevice, renderer->depthImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->depthImageView, NULL);
    free(renderer->swapImages);
    free(renderer->swapImageViews);
    free(renderer->framebuffers);

    /* DESCRIPTOR SETS */
    vkDestroyDescriptorPool(
        logicalDevice,
        renderer->descriptorPool,
        NULL);
    vkDestroyDescriptorSetLayout(
        logicalDevice,
        renderer->renderDescriptorSetLayout,
        NULL);
    free(renderer->renderDescriptorSets);
    vkDestroyBuffer(
        logicalDevice,
        renderer->renderDescriptorBuffer,
        NULL);
    vkFreeMemory(
        logicalDevice,
        renderer->renderDescriptorBufferMemory,
        NULL);

    /* PIPELINES */
    vkDestroyRenderPass(logicalDevice, renderer->objRenderPass, NULL);
    vkDestroyPipelineLayout(logicalDevice, renderer->pipelineLayout, NULL);
    vkDestroyPipeline(logicalDevice, renderer->pipeline, NULL);

    /* COMMAND BUFFERS */
    free(renderer->commandBuffers);

    /* SYNCHRONIZATION */
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(logicalDevice, renderer->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(logicalDevice, renderer->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(logicalDevice, renderer->renderFinishedFences[i], NULL);
    }
    free(renderer->swapchainImageFences);
}
