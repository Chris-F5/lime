#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 model;
  mat4 view;
  mat4 proj;
};
layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void
main()
{
  out_color = texture(texture_sampler, in_uv);
}
