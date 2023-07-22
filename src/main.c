#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "lime.h"
#include "camera.h"

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

  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "lime demo", NULL, NULL);

  lime_init_device(window);
  lime_init_pipelines();
  lime_init_resources();
  lime_init_renderer();
  printf("hello world\n");

  camera.x = camera.y = camera.z = 0.0f;
  camera.yaw = camera.pitch = 0.0f;
  mat4_projection(camera_uniform_data.proj, 1, 1.5f, 0.1, 100);
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
  lime_destroy_resources();
  lime_destroy_pipelines();
  lime_destroy_device();
  return 0;
}
