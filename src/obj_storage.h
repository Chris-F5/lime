#ifndef LIME_OBJ_STORAGE_H
#define LIME_OBJ_STORAGE_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

#define OBJECT_CAPACITY (50)

#define MAX_OBJ_SCALE 256
#define MAX_OBJ_VOX_COUNT (MAX_OBJ_SCALE * MAX_OBJ_SCALE * MAX_OBJ_SCALE)

typedef uint32_t ObjRef;

typedef struct {
    mat4 model;
} ObjectUniformBuffer;

typedef struct {
    uint32_t filled;

    vec3* positions;
    ivec3* sizes;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkBuffer voxImageStagingBuffer;
    VkDeviceMemory voxImageStagingBufferMemory;

    VkImage* voxColorImages;
    VkImageView* voxColorImageViews;
    VkDeviceMemory* voxColorImagesMemory;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
} ObjectStorage;

void ObjectStorage_init(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice);

void ObjectStorage_addObjects(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientCommandPool,
    uint32_t count,
    vec3* positions,
    ivec3* sizes,
    ObjRef* objRefs);

void ObjectStorage_updateVoxColors(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkCommandPool transientCommandPool,
    VkQueue graphicsQueue,
    ObjRef obj,
    uint8_t* colors);

void ObjectStorage_destroy(ObjectStorage* storage, VkDevice logicalDevice);

#endif
