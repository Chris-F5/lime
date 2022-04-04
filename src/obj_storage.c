#include "./obj_storage.h"

#include <stdlib.h>

#include <cglm/cglm.h>

#include "vk_utils.h"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define FACE_XP_BIT 1
#define FACE_YP_BIT 2
#define FACE_ZP_BIT 4
#define FACE_XN_BIT 8
#define FACE_YN_BIT 16
#define FACE_ZN_BIT 32

const static VkVertexInputBindingDescription OBJ_VERT_BINDINGS[] = {
    { 0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX }
};

const static VkVertexInputAttributeDescription OBJ_VERT_INPUT_ATTRIBS[] = {
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectVertex, pos) },
};

const static VkFormat VOX_COLOR_IMAGE_FORMAT = VK_FORMAT_R8_UINT;

const static vec3 cubePoints[] = {
    { 0, 0, 0 },
    { 0, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 1, 1 }
};

void getObjectVertexInfo(
    uint32_t* bindingCount,
    const VkVertexInputBindingDescription** bindings,
    uint32_t* attributeCount,
    const VkVertexInputAttributeDescription** attributes)
{
    *bindingCount = sizeof(OBJ_VERT_BINDINGS)
        / sizeof(OBJ_VERT_BINDINGS[0]);
    *bindings = OBJ_VERT_BINDINGS;
    *attributeCount = sizeof(OBJ_VERT_INPUT_ATTRIBS)
        / sizeof(OBJ_VERT_INPUT_ATTRIBS[0]);
    *attributes = OBJ_VERT_INPUT_ATTRIBS;
}

void createHexahedronObjectVerts(
    vec3* points,
    ObjectVertex* verts)
{
    glm_vec3_copy(points[0], verts[0].pos);
    glm_vec3_copy(points[2], verts[1].pos);
    glm_vec3_copy(points[3], verts[2].pos);
    glm_vec3_copy(points[0], verts[3].pos);
    glm_vec3_copy(points[3], verts[4].pos);
    glm_vec3_copy(points[1], verts[5].pos);

    glm_vec3_copy(points[4], verts[6].pos);
    glm_vec3_copy(points[7], verts[7].pos);
    glm_vec3_copy(points[6], verts[8].pos);
    glm_vec3_copy(points[4], verts[9].pos);
    glm_vec3_copy(points[5], verts[10].pos);
    glm_vec3_copy(points[7], verts[11].pos);

    glm_vec3_copy(points[0], verts[12].pos);
    glm_vec3_copy(points[6], verts[13].pos);
    glm_vec3_copy(points[2], verts[14].pos);
    glm_vec3_copy(points[0], verts[15].pos);
    glm_vec3_copy(points[4], verts[16].pos);
    glm_vec3_copy(points[6], verts[17].pos);

    glm_vec3_copy(points[1], verts[18].pos);
    glm_vec3_copy(points[3], verts[19].pos);
    glm_vec3_copy(points[7], verts[20].pos);
    glm_vec3_copy(points[1], verts[21].pos);
    glm_vec3_copy(points[7], verts[22].pos);
    glm_vec3_copy(points[5], verts[23].pos);

    glm_vec3_copy(points[0], verts[24].pos);
    glm_vec3_copy(points[1], verts[25].pos);
    glm_vec3_copy(points[5], verts[26].pos);
    glm_vec3_copy(points[0], verts[27].pos);
    glm_vec3_copy(points[5], verts[28].pos);
    glm_vec3_copy(points[4], verts[29].pos);

    glm_vec3_copy(points[2], verts[30].pos);
    glm_vec3_copy(points[7], verts[31].pos);
    glm_vec3_copy(points[3], verts[32].pos);
    glm_vec3_copy(points[2], verts[33].pos);
    glm_vec3_copy(points[6], verts[34].pos);
    glm_vec3_copy(points[7], verts[35].pos);
}

void createObjectDescriptorSetLayout(
    VkDevice logicalDevice,
    VkDescriptorSetLayout* layout)
{
    VkDescriptorSetLayoutBinding uboBinding;
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags
        = VK_SHADER_STAGE_VERTEX_BIT
        | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding colorBinding;
    colorBinding.binding = 1;
    colorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    colorBinding.descriptorCount = 1;
    colorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    colorBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding bindings[] = {
        uboBinding, colorBinding
    };
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.sType
        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pNext = NULL;
    layoutCreateInfo.flags = 0;
    layoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutCreateInfo.pBindings = bindings;
    handleVkResult(
        vkCreateDescriptorSetLayout(
            logicalDevice,
            &layoutCreateInfo,
            NULL,
            layout),
        "creating object descriptor set layout");
}

void createObjectDescriptorPool(
    VkDevice logicalDevice,
    VkDescriptorPool* pool)
{
    VkDescriptorPoolSize uboSize;
    uboSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboSize.descriptorCount = OBJECT_CAPACITY;

    VkDescriptorPoolSize colorSize;
    colorSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    colorSize.descriptorCount = OBJECT_CAPACITY;

    VkDescriptorPoolSize poolSizes[] = {
        uboSize, colorSize
    };
    VkDescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.pNext = NULL;
    poolCreateInfo.flags = 0;
    poolCreateInfo.maxSets = OBJECT_CAPACITY;
    poolCreateInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolCreateInfo.pPoolSizes = poolSizes;

    handleVkResult(
        vkCreateDescriptorPool(
            logicalDevice,
            &poolCreateInfo,
            NULL,
            pool),
        "creating object descriptor pool");
}

void createVoxelColorImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientCommandPool,
    VkExtent3D extent,
    VkImage* image,
    VkDeviceMemory* imageMemory,
    VkImageView* imageView)
{
    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = NULL;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
    imageCreateInfo.format = VOX_COLOR_IMAGE_FORMAT;
    imageCreateInfo.extent = extent;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = NULL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    handleVkResult(
        vkCreateImage(
            logicalDevice,
            &imageCreateInfo,
            NULL,
            image),
        "creating object voxel color image");

    allocateImageMemory(
        logicalDevice,
        physicalDevice,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        *image,
        imageMemory);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    VkImageViewCreateInfo colorImageViewCreateInfo;
    colorImageViewCreateInfo.sType
        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageViewCreateInfo.pNext = NULL;
    colorImageViewCreateInfo.flags = 0;
    colorImageViewCreateInfo.image = *image;
    colorImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
    colorImageViewCreateInfo.format = VOX_COLOR_IMAGE_FORMAT;
    colorImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorImageViewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &colorImageViewCreateInfo,
            NULL,
            imageView),
        "creating object color image view");

    transitionImageLayout(
        logicalDevice,
        graphicsQueue,
        transientCommandPool,
        *image,
        subresourceRange,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        0,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

void ObjectStorage_init(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice)
{
    storage->filled = 0;
    storage->positions = (vec3*)malloc(
        OBJECT_CAPACITY * sizeof(vec3));
    storage->sizes = (ivec3*)malloc(
        OBJECT_CAPACITY * sizeof(ivec3));
    storage->vertBufferOffsets = (uint32_t*)malloc(
        OBJECT_CAPACITY * sizeof(uint32_t));
    storage->voxColorImages = (VkImage*)malloc(
        OBJECT_CAPACITY * sizeof(VkImage));
    storage->voxColorImageViews = (VkImageView*)malloc(
        OBJECT_CAPACITY * sizeof(VkImageView));
    storage->voxColorImagesMemory = (VkDeviceMemory*)malloc(
        OBJECT_CAPACITY * sizeof(VkDeviceMemory));
    storage->descriptorSets = (VkDescriptorSet*)malloc(
        OBJECT_CAPACITY * sizeof(VkDescriptorSet));

    /* GPU BUFFERS */

    createBuffer(
        logicalDevice,
        physicalDevice,
        OBJECT_CAPACITY
            * MAX_OBJ_VERT_COUNT
            * sizeof(ObjectVertex),
        0,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &storage->vertBuffer,
        &storage->vertBufferMemory);

    createBuffer(
        logicalDevice,
        physicalDevice,
        OBJECT_CAPACITY * sizeof(ObjectUniformBuffer),
        0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &storage->uniformBuffer,
        &storage->uniformBufferMemory);

    /* DESCRIPTOR SETS */
    createObjectDescriptorSetLayout(
        logicalDevice,
        &storage->descriptorSetLayout);
    createObjectDescriptorPool(
        logicalDevice,
        &storage->descriptorPool);

    for (uint32_t i = 0; i < OBJECT_CAPACITY; i++)
        storage->descriptorSets[i] = VK_NULL_HANDLE;
}

void ObjectStorage_addObjects(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientCommandPool,
    uint32_t count,
    vec3* positions,
    ivec3* sizes,
    ObjRef* objRefs)
{
    for (uint32_t i = 0; i < count; i++)
        objRefs[i] = storage->filled++;
    for (uint32_t i = 0; i < count; i++) {
        ObjRef obj = objRefs[i];

        /* POSITION AND SIZE */
        glm_vec3_copy(positions[i], storage->positions[obj]);
        storage->sizes[obj][0] = sizes[i][0];
        storage->sizes[obj][1] = sizes[i][1];
        storage->sizes[obj][2] = sizes[i][2];

        /* VERTS */
        storage->vertBufferOffsets[obj]
            = obj * MAX_OBJ_VERT_COUNT;
        {
            ObjectVertex* verts;
            handleVkResult(
                vkMapMemory(
                    logicalDevice,
                    storage->vertBufferMemory,
                    storage->vertBufferOffsets[obj] * sizeof(ObjectVertex),
                    MAX_OBJ_VERT_COUNT * sizeof(ObjectVertex),
                    0,
                    (void*)&verts),
                "mapping object vertex memory");
            createHexahedronObjectVerts(
                (vec3*)cubePoints,
                verts);
            vkUnmapMemory(logicalDevice, storage->vertBufferMemory);
        }

        /* IMAGE */
        createVoxelColorImage(
            logicalDevice,
            physicalDevice,
            graphicsQueue,
            transientCommandPool,
            (VkExtent3D) {
                storage->sizes[obj][0],
                storage->sizes[obj][1],
                storage->sizes[obj][2] },
            &storage->voxColorImages[obj],
            &storage->voxColorImagesMemory[obj],
            &storage->voxColorImageViews[obj]);

        /* UNIFORM BUFFER */
        allocateDescriptorSets(
            logicalDevice,
            storage->descriptorSetLayout,
            storage->descriptorPool,
            1,
            &storage->descriptorSets[obj]);
        VkDescriptorBufferInfo uboInfo;
        uboInfo.buffer = storage->uniformBuffer;
        uboInfo.offset = obj * sizeof(ObjectUniformBuffer);
        uboInfo.range = sizeof(ObjectUniformBuffer);

        VkWriteDescriptorSet uboDescriptorSetWrite;
        uboDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboDescriptorSetWrite.pNext = NULL;
        uboDescriptorSetWrite.dstSet = storage->descriptorSets[obj];
        uboDescriptorSetWrite.dstBinding = 0;
        uboDescriptorSetWrite.dstArrayElement = 0;
        uboDescriptorSetWrite.descriptorCount = 1;
        uboDescriptorSetWrite.descriptorType
            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescriptorSetWrite.pImageInfo = NULL;
        uboDescriptorSetWrite.pBufferInfo = &uboInfo;
        uboDescriptorSetWrite.pTexelBufferView = NULL;

        VkDescriptorImageInfo colorInfo;
        colorInfo.sampler = VK_NULL_HANDLE;
        colorInfo.imageView = storage->voxColorImageViews[obj];
        colorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet colorDescriptorSetWrite;
        colorDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorDescriptorSetWrite.pNext = NULL;
        colorDescriptorSetWrite.dstSet = storage->descriptorSets[obj];
        colorDescriptorSetWrite.dstBinding = 1;
        colorDescriptorSetWrite.dstArrayElement = 0;
        colorDescriptorSetWrite.descriptorCount = 1;
        colorDescriptorSetWrite.descriptorType
            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        colorDescriptorSetWrite.pImageInfo = &colorInfo;
        colorDescriptorSetWrite.pBufferInfo = NULL;
        colorDescriptorSetWrite.pTexelBufferView = NULL;
        VkWriteDescriptorSet setWrites[] = {
            uboDescriptorSetWrite, colorDescriptorSetWrite
        };
        vkUpdateDescriptorSets(
            logicalDevice,
            sizeof(setWrites) / sizeof(setWrites[0]),
            setWrites,
            0,
            NULL);

        ObjectUniformBuffer* uniformBuffer;
        handleVkResult(
            vkMapMemory(
                logicalDevice,
                storage->uniformBufferMemory,
                obj * sizeof(ObjectUniformBuffer),
                sizeof(ObjectUniformBuffer),
                0,
                (void**)&uniformBuffer),
            "mapping object uniform buffer memory");
        glm_mat4_identity(uniformBuffer->model);
        glm_translate(uniformBuffer->model, storage->positions[obj]);
        vec3 objScale = {
            storage->sizes[obj][0],
            storage->sizes[obj][1],
            storage->sizes[obj][2]
        };
        glm_scale(uniformBuffer->model, objScale);
        vkUnmapMemory(
            logicalDevice,
            storage->uniformBufferMemory);
    }
}

void ObjectStorage_updateVoxColors(
    ObjectStorage* storage,
    ObjRef obj,
    uint8_t* colors)
{
}

void ObjectStorage_destroy(ObjectStorage* storage, VkDevice logicalDevice)
{
    vkDestroyBuffer(logicalDevice, storage->vertBuffer, NULL);
    vkFreeMemory(logicalDevice, storage->vertBufferMemory, NULL);
    vkDestroyBuffer(logicalDevice, storage->uniformBuffer, NULL);
    vkFreeMemory(logicalDevice, storage->uniformBufferMemory, NULL);

    for (int i = 0; i < storage->filled; i++) {
        vkDestroyImage(logicalDevice, storage->voxColorImages[i], NULL);
        vkDestroyImageView(logicalDevice, storage->voxColorImageViews[i], NULL);
        vkFreeMemory(logicalDevice, storage->voxColorImagesMemory[i], NULL);
    }

    vkDestroyDescriptorPool(
        logicalDevice,
        storage->descriptorPool,
        NULL);

    vkDestroyDescriptorSetLayout(
        logicalDevice,
        storage->descriptorSetLayout,
        NULL);

    free(storage->positions);
    free(storage->sizes);
    free(storage->vertBufferOffsets);
    free(storage->voxColorImages);
    free(storage->voxColorImageViews);
    free(storage->voxColorImagesMemory);
}
