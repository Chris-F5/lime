#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 view;
  mat4 proj;
  int number;
};

vec3 hello_triangle[3] = vec3[] (
  vec3(-0.5, 0.5, 5.0),
  vec3(0.5, 0.5, 5.0),
  vec3(0.0, -0.5, 5.0)
);

void
main()
{
  gl_Position = proj * view * vec4(hello_triangle[gl_VertexIndex], 1.0);
}
