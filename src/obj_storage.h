#ifndef LIME_OBJ_STORAGE_H
#define LIME_OBJ_STORAGE_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

#define OBJECT_CAPACITY (50)

#define MAX_OBJ_BACK_FACE_VERT_COUNT 36

typedef uint32_t ObjRef;

typedef struct {
    vec4 pos;
    uint32_t obj;
} ObjectBackFaceVertex;

typedef struct {
    uint32_t filled;

    vec3* positions;
    ivec3* sizes;
    uint32_t* backFaceVertBufferOffsets;
    uint32_t* backFaceVertCounts;
    unsigned char* backFaceBits;

    VkBuffer backFaceVertBuffer;
    VkDeviceMemory backFaceVertMemory;
} ObjectStorage;

void getObjectBackFaceVertexInfo(
    uint32_t* bindingCount,
    const VkVertexInputBindingDescription** bindings,
    uint32_t* attributeCount,
    const VkVertexInputAttributeDescription** attributes);

void ObjectStorage_init(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice);

void ObjectStorage_addObjects(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    uint32_t count,
    vec3* positions,
    ivec3* sizes,
    ObjRef* objRefs);

void ObjectStorage_updateCamPos(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    vec3 camPos);

void ObjectStorage_destroy(ObjectStorage* storage, VkDevice logicalDevice);

#endif
