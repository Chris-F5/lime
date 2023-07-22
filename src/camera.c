#include <GLFW/glfw3.h>
#include "camera.h"

#include <math.h>

const float rot_speed = 0.05f;
const float move_speed = 0.2f;

void
process_camera_input(struct camera *camera, GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
      camera->yaw += rot_speed;
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
      camera->yaw -= rot_speed;
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
      camera->pitch += rot_speed;
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
      camera->pitch -= rot_speed;
  if (camera->pitch > M_PI / 2.0f)
    camera->pitch = M_PI / 2.0f;
  if (camera->pitch < -M_PI / 2.0f)
    camera->pitch = -M_PI / 2.0f;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera->x -= sinf(camera->yaw) * move_speed;
    camera->z += cosf(camera->yaw) * move_speed;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera->x += sinf(camera->yaw) * move_speed;
    camera->z -= cosf(camera->yaw) * move_speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera->x -= sinf(camera->yaw + M_PI / 2.0f) * move_speed;
    camera->z += cosf(camera->yaw + M_PI / 2.0f) * move_speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera->x += sinf(camera->yaw + M_PI / 2.0f) * move_speed;
    camera->z -= cosf(camera->yaw + M_PI / 2.0f) * move_speed;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    camera->y += move_speed;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    camera->y -= move_speed;
}
