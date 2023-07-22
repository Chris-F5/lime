typedef float mat4[16];

void mat4_identity(mat4 m);
void mat4_projection(mat4 m, float aspect_ratio, float vertical_fov, float near, float far);
