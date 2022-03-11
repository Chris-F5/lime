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

const static VkVertexInputBindingDescription BACK_FACE_VERT_BINDINGS[] = {
    { 0, sizeof(ObjectBackFaceVertex), VK_VERTEX_INPUT_RATE_VERTEX }
};

const static VkVertexInputAttributeDescription BACK_FACE_VERT_INPUT_ATTRIBS[] = {
    { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ObjectBackFaceVertex, pos) },
};

void getObjectBackFaceVertexInfo(
    uint32_t* bindingCount,
    const VkVertexInputBindingDescription** bindings,
    uint32_t* attributeCount,
    const VkVertexInputAttributeDescription** attributes)
{
    *bindingCount = sizeof(BACK_FACE_VERT_BINDINGS)
        / sizeof(BACK_FACE_VERT_BINDINGS[0]);
    *bindings = BACK_FACE_VERT_BINDINGS;
    *attributeCount = sizeof(BACK_FACE_VERT_INPUT_ATTRIBS)
        / sizeof(BACK_FACE_VERT_INPUT_ATTRIBS[0]);
    *attributes = BACK_FACE_VERT_INPUT_ATTRIBS;
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
    storage->backFaceVertBufferOffsets = (uint32_t*)malloc(
        OBJECT_CAPACITY * sizeof(uint32_t));
    storage->backFaceVertCounts = (uint32_t*)malloc(
        OBJECT_CAPACITY * sizeof(uint32_t));
    storage->backFaceBits = malloc(OBJECT_CAPACITY);

    createBuffer(
        logicalDevice,
        physicalDevice,
        OBJECT_CAPACITY
            * MAX_OBJ_BACK_FACE_VERT_COUNT
            * sizeof(ObjectBackFaceVertex),
        0,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &storage->backFaceVertBuffer,
        &storage->backFaceVertMemory);
}

static void face6Verts(
    vec3 position,
    ivec3 size,
    unsigned char faceBit,
    ObjectBackFaceVertex* verts)
{
    uint32_t axis;
    if (faceBit & (FACE_XP_BIT | FACE_XN_BIT))
        axis = 0;
    else if (faceBit & (FACE_YP_BIT | FACE_YN_BIT))
        axis = 1;
    else if (faceBit & (FACE_ZP_BIT | FACE_ZN_BIT))
        axis = 2;
    else {
        puts("Invalid face bit. Exiting.");
        exit(1);
    }

    vec3 adjustedPosition;
    glm_vec3_copy(position, adjustedPosition);
    uint32_t adjusted = 0;
    if (faceBit & (FACE_XP_BIT | FACE_YP_BIT | FACE_ZP_BIT)) {
        adjustedPosition[axis] += size[axis];
        adjusted = 1;
    }

    for (uint32_t i = 0; i < 6; i++) {
        glm_vec3_copy(adjustedPosition, verts[i].pos);
        verts[i].pos[3] = 1.0;
    }
    verts[1 + adjusted].pos[(axis + 1) % 3] += size[(axis + 1) % 3];
    verts[2 - adjusted].pos[(axis + 2) % 3] += size[(axis + 2) % 3];
    verts[3].pos[(axis + 2) % 3] += size[(axis + 2) % 3];
    verts[4 + adjusted].pos[(axis + 1) % 3] += size[(axis + 1) % 3];
    verts[5 - adjusted].pos[(axis + 1) % 3] += size[(axis + 1) % 3];
    verts[5 - adjusted].pos[(axis + 2) % 3] += size[(axis + 2) % 3];
}

static unsigned char getBackFaceBits(vec3 camPos, vec3 position, ivec3 size)
{
    unsigned char backFaceBits = 0;
    if (camPos[0] > position[0])
        backFaceBits |= FACE_XN_BIT;
    if (camPos[0] < position[0] + size[0])
        backFaceBits |= FACE_XP_BIT;
    if (camPos[1] > position[1])
        backFaceBits |= FACE_YN_BIT;
    if (camPos[1] < position[1] + size[1])
        backFaceBits |= FACE_YP_BIT;
    if (camPos[2] > position[2])
        backFaceBits |= FACE_ZN_BIT;
    if (camPos[2] < position[2] + size[2])
        backFaceBits |= FACE_ZP_BIT;
    return backFaceBits;
}

static void objBackFacesVerts(
    unsigned char backFaceBits,
    vec3 position,
    ivec3 size,
    uint32_t* vertCount,
    ObjectBackFaceVertex* verts)
{
    uint32_t c = 0;
    if (backFaceBits & FACE_XP_BIT) {
        face6Verts(
            position,
            size,
            FACE_XP_BIT,
            &verts[c]);
        c += 6;
    }
    if (backFaceBits & FACE_XN_BIT) {
        face6Verts(
            position,
            size,
            FACE_XN_BIT,
            &verts[c]);
        c += 6;
    }
    if (backFaceBits & FACE_YP_BIT) {
        face6Verts(
            position,
            size,
            FACE_YP_BIT,
            &verts[c]);
        c += 6;
    }
    if (backFaceBits & FACE_YN_BIT) {
        face6Verts(
            position,
            size,
            FACE_YN_BIT,
            &verts[c]);
        c += 6;
    }
    if (backFaceBits & FACE_ZP_BIT) {
        face6Verts(
            position,
            size,
            FACE_ZP_BIT,
            &verts[c]);
        c += 6;
    }
    if (backFaceBits & FACE_ZN_BIT) {
        face6Verts(
            position,
            size,
            FACE_ZN_BIT,
            &verts[c]);
        c += 6;
    }
    *vertCount = c;
    while(c < MAX_OBJ_BACK_FACE_VERT_COUNT) {
        verts[c].pos[3] = 0.0;
        c += 1;
    }
}

void ObjectStorage_addObjects(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    uint32_t count,
    vec3* positions,
    ivec3* sizes,
    ObjRef* objRefs)
{
    for (uint32_t i = 0; i < count; i++)
        objRefs[i] = storage->filled++;
    for (uint32_t i = 0; i < count; i++) {
        glm_vec3_copy(positions[i], storage->positions[objRefs[i]]);
        storage->sizes[objRefs[i]][0] = sizes[i][0];
        storage->sizes[objRefs[i]][1] = sizes[i][1];
        storage->sizes[objRefs[i]][2] = sizes[i][2];
        storage->backFaceVertBufferOffsets[i]
            = objRefs[i] * MAX_OBJ_BACK_FACE_VERT_COUNT;
        storage->backFaceVertCounts[objRefs[i]] = 0;
        storage->backFaceBits[objRefs[i]] = 0;
    }
}

void ObjectStorage_updateCamPos(
    ObjectStorage* storage,
    VkDevice logicalDevice,
    vec3 camPos)
{
    ObjectBackFaceVertex* verts;
    handleVkResult(
        vkMapMemory(
            logicalDevice,
            storage->backFaceVertMemory,
            0,
            storage->filled
                * MAX_OBJ_BACK_FACE_VERT_COUNT
                * sizeof(ObjectBackFaceVertex),
            0,
            (void**)&verts),
        "mapping vertex buffer memory");
    for (uint32_t i = 0; i < storage->filled; i++) {
        unsigned char newBackFaceBits = getBackFaceBits(
            camPos,
            storage->positions[i],
            storage->sizes[i]);
        if (newBackFaceBits != storage->backFaceBits[i]) {
            storage->backFaceBits[i] = newBackFaceBits;
            objBackFacesVerts(
                storage->backFaceBits[i],
                storage->positions[i],
                storage->sizes[i],
                &storage->backFaceVertCounts[i],
                &verts[i * 18]);
        }
    }
    vkUnmapMemory(logicalDevice, storage->backFaceVertMemory);
}

void ObjectStorage_destroy(ObjectStorage* storage, VkDevice logicalDevice)
{
    free(storage->positions);
    free(storage->sizes);
    free(storage->backFaceVertBufferOffsets);
    free(storage->backFaceVertCounts);
    free(storage->backFaceBits);

    vkDestroyBuffer(logicalDevice, storage->backFaceVertBuffer, NULL);
    vkFreeMemory(logicalDevice, storage->backFaceVertMemory, NULL);
}
