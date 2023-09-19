#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "obj_types.h"
#include "block_allocation.h"
#include "lime.h"
#include "utils.h"
#include "camera.h"
#include "obj.h"

static void glfw_error_callback(int _, const char* errorString);

static const uint32_t WIDTH = 800;
static const uint32_t HEIGHT = 800;

static void
glfw_error_callback(int _, const char* str)
{
  printf("glfw error: '%s'\n", str);
  exit(1);
}

int
main(int argc, char **argv)
{
  GLFWwindow *window;
  struct wavefront_obj wavefront;
  struct indexed_vertex_obj ivo;
  struct graphics_vertex_obj gvo;
  struct camera camera;
  struct camera_uniform_data camera_uniform_data;
  struct voxel_block_uniform_data block_uniform_data;
  int block_size, i;
  char *voxels;

  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "lime demo", NULL, NULL);

  load_wavefront_obj(&wavefront, "viking_room.obj");
  wavefront_to_indexed_vertex_obj(&ivo, &wavefront);
  block_size = 16;
  voxels = xmalloc(block_size * block_size * block_size);
  for (i = 0; i < block_size * block_size * block_size; i++)
    voxels[i] = (i % 10 == 0) ? 1 : 0;
  mat4_view(block_uniform_data.model, 0.0f, 0.0f, -0.4f, 0.0f, -0.55f);
  block_uniform_data.scale = 16;

  lime_init_device(window);
  lime_init_pipelines();
  lime_init_resources();
  lime_init_vertex_buffers(32000, 32000);
  lime_create_graphics_vertex_obj(&gvo, &ivo);
  lime_init_textures("viking_room.png");
  lime_init_voxel_blocks(block_uniform_data, block_size, voxels);
  lime_init_renderer(&gvo);

  destroy_wavefront_obj(&wavefront);
  destroy_indexed_vertex_obj(&ivo);
  free(voxels);

  camera.x = camera.y = camera.z = 0.0f;
  camera.yaw = camera.pitch = 0.0f;
  mat4_projection(camera_uniform_data.proj, 1.0f, 1.5f, 0.1f, 100.0f);
  mat4_view(camera_uniform_data.model, 3.141592f * 1.5f, 0.0f, 0.0f, 0.0f, 0.0f);
  camera_uniform_data.color = 0;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    process_camera_input(&camera, window);
    mat4_view(camera_uniform_data.view, camera.pitch, camera.yaw, camera.x, camera.y, camera.z);
    lime_draw_frame(camera_uniform_data);
    camera_uniform_data.color = (camera_uniform_data.color + 1) % 256;
  }
  vkDeviceWaitIdle(lime_device.device);
  lime_destroy_renderer();
  lime_destroy_voxel_blocks();
  lime_destroy_textures();
  lime_destroy_vertex_buffers();
  lime_destroy_resources();
  lime_destroy_pipelines();
  lime_destroy_device();
  return 0;
}
