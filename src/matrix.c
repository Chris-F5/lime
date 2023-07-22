#include "matrix.h"
#include <math.h>

void
mat4_identity(mat4 m)
{
  m[ 0] = 1.0f;
  m[ 1] = 0.0f;
  m[ 2] = 0.0f;
  m[ 3] = 0.0f;

  m[ 4] = 0.0f;
  m[ 5] = 1.0f;
  m[ 6] = 0.0f;
  m[ 7] = 0.0f;

  m[ 8] = 0.0f;
  m[ 9] = 0.0f;
  m[10] = 1.0f;
  m[11] = 0.0f;

  m[12] = 0.0f;
  m[13] = 0.0f;
  m[14] = 0.0f;
  m[15] = 1.0f;
}

void
mat4_projection(mat4 m, float aspect_ratio, float vertical_fov, float near, float far)
{
  float cot;
  cot = 1.0f / tanf(vertical_fov / 2.0f);

  m[ 0] = cot / aspect_ratio;
  m[ 1] = 0.0f;
  m[ 2] = 0.0f;
  m[ 3] = 0.0f;

  m[ 4] = 0.0f;
  m[ 5] = -cot;
  m[ 6] = 0.0f;
  m[ 7] = 0.0f;

  m[ 8] = 0.0f;
  m[ 9] = 0.0f;
  m[10] = far / (far - near);
  m[11] = 1.0f;

  m[12] = 0.0f;
  m[13] = 0.0f;
  m[14] = -(near * far) / (far - near);
  m[15] = 0.0f;
}

void
mat4_view(mat4 m, float pitch, float yaw, float x, float y, float z)
{
  float cosy, siny, cosp, sinp;
  cosy = cosf(yaw);
  siny = sinf(yaw);
  cosp = cosf(pitch);
  sinp = sinf(pitch);

  m[ 0] = cosy;
  m[ 1] = -sinp * siny;
  m[ 2] = cosp * siny;
  m[ 3] = 0.0f;

  m[ 4] = 0.0f;
  m[ 5] = cosp;
  m[ 6] = sinp;
  m[ 7] = 0.0f;

  m[ 8] = -siny;
  m[ 9] = -sinp * cosy;
  m[10] = cosp * cosy;
  m[11] = 0.0f;

  m[12] = -x * m[0] - y * m[4] - z * m[8];
  m[13] = -x * m[1] - y * m[5] - z * m[9];
  m[14] = -x * m[2] - y * m[6] - z * m[10];
  m[15] = 1.0f;
}
