struct camera {
  float x, y, z;
  float yaw, pitch;
};

void process_camera_input(struct camera *camera, GLFWwindow *window);
