#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 model;
  mat4 view;
  mat4 proj;
};

layout(location = 0) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void
main()
{
  out_color = in_color;
}
