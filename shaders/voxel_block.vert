#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 old_model;
  mat4 view;
  mat4 proj;
};

layout(set = 1, binding = 0) uniform block_uniform_buffer {
    mat4 model;
};

layout(location = 0) out vec3 out_pos;

void
main()
{
  gl_Position = proj * view * model * vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
