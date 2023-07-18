#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  int number;
};

layout(location = 0) out vec4 out_color;

void
main()
{
  out_color = vec4(1.0, float(number) / 255.0f, 0.0, 1.0);
}
