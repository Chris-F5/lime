#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 model;
  mat4 view;
  mat4 proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;

void
main()
{
  gl_Position = proj * view * model * vec4(in_position, 1.0f);
  out_color = in_normal;
}
