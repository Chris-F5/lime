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

  init_video(window);
  printf("hello world\n");

  while (!glfwWindowShouldClose(window))
    glfwPollEvents();
  destroy_video();
  return 0;
}
