#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static void glfw_error_callback(int _, const char* errorString);

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

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
  VkSurfaceKHR surface;
  VkResult err;
  struct lime_renderer renderer;

  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "lime demo", NULL, NULL);

  create_renderer(&renderer, window);
  printf("hello world\n");

  while (!glfwWindowShouldClose(window))
    glfwPollEvents();
  destroy_renderer(&renderer);
  return 0;
}
