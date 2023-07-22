#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
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
  struct camera camera;
  struct camera_uniform_data camera_uniform_data;
  int vertex_count, face_count, i;
  float *vertex_positions, *vertex_colors;
  uint32_t *vertex_indices;

  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "lime demo", NULL, NULL);

  parse_obj("viking_room.obj", &vertex_count, &face_count, &vertex_positions, &vertex_indices);
  vertex_colors = xmalloc(vertex_count * 4 * sizeof(float));
  for (i = 0; i < vertex_count; i++) {
    vertex_colors[i * 4 + 0] = (float)rand() / (float)RAND_MAX;
    vertex_colors[i * 4 + 1] = (float)rand() / (float)RAND_MAX;
    vertex_colors[i * 4 + 2] = (float)rand() / (float)RAND_MAX;
    vertex_colors[i * 4 + 3] = 1.0f;
  }

  lime_init_device(window);
  lime_init_pipelines();
  lime_init_resources();
  lime_init_vertex_buffers(vertex_count, face_count, vertex_positions, vertex_colors, vertex_indices);
  lime_init_renderer();

  free(vertex_positions);
  free(vertex_colors);
  free(vertex_indices);

  camera.x = camera.y = camera.z = 0.0f;
  camera.yaw = camera.pitch = 0.0f;
  mat4_projection(camera_uniform_data.proj, 1, 1.5f, 0.1, 100);
  mat4_view(camera_uniform_data.model, 3.141592f * 1.5, 0, 0, 0, 0);
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
  lime_destroy_vertex_buffers();
  lime_destroy_resources();
  lime_destroy_pipelines();
  lime_destroy_device();
  return 0;
}
