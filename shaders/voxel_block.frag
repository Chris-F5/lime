#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 old_model;
  mat4 view;
  mat4 proj;
};

layout(set = 1, binding = 0) uniform block_uniform_buffer {
    mat4 model;
};
layout(set = 1, binding = 1, r8ui) uniform uimage3D block_voxels;

layout(location = 0) in vec3 in_pos;

layout(location = 0) out vec4 out_color;

void
main()
{
  out_color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
}
