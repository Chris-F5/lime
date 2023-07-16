#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    lime_draw_frame();
  }
  vkDeviceWaitIdle(lime_device.device);
  lime_destroy_renderer();
  lime_destroy_resources();
  lime_destroy_pipelines();
  lime_destroy_device();
  return 0;
}
