#ifndef XSHARP_MATH3D_H
#define XSHARP_MATH3D_H

#include <math.h>
#include <stdbool.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(d) ((d) * (M_PI / 180.0f))
#define RAD2DEG(r) ((r) * (180.0f / M_PI))
#define XSHARP_EPSILON 1e-6f

/* ------------------------------------------------------------------ */
/*  Vector types                                                      */
/* ------------------------------------------------------------------ */

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;

/* 4x4 column-major matrix (m[col][row]) */
typedef struct { float m[4][4]; } Mat4;

typedef struct { float x, y, z, w; } Quat;

/* Axis-aligned bounding box */
typedef struct { Vec3 min, max; } AABB;

/* Ray */
typedef struct { Vec3 origin, dir; } Ray;

/* Plane  (normal.dot(p) + d = 0) */
typedef struct { Vec3 normal; float d; } Plane;

/* Frustum – six planes */
typedef struct { Plane planes[6]; } Frustum;

/* ------------------------------------------------------------------ */
/*  Vec2                                                               */
/* ------------------------------------------------------------------ */

static inline Vec2 vec2(float x, float y) { return (Vec2){x, y}; }
static inline Vec2 vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x+b.x, a.y+b.y}; }
static inline Vec2 vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x-b.x, a.y-b.y}; }
static inline Vec2 vec2_mul(Vec2 a, float s) { return (Vec2){a.x*s, a.y*s}; }
static inline Vec2 vec2_div(Vec2 a, float s) { return (Vec2){a.x/s, a.y/s}; }
static inline float vec2_dot(Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
static inline float vec2_length(Vec2 v) { return sqrtf(vec2_dot(v,v)); }
static inline Vec2 vec2_normalize(Vec2 v) { float l=vec2_length(v); return l>XSHARP_EPSILON? vec2_div(v,l):v; }
static inline Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) { return vec2_add(vec2_mul(a,1-t), vec2_mul(b,t)); }

/* ------------------------------------------------------------------ */
/*  Vec3                                                               */
/* ------------------------------------------------------------------ */

static inline Vec3 vec3(float x, float y, float z) { return (Vec3){x,y,z}; }
static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vec3 vec3_mul(Vec3 a, float s) { return (Vec3){a.x*s, a.y*s, a.z*s}; }
static inline Vec3 vec3_div(Vec3 a, float s) { return (Vec3){a.x/s, a.y/s, a.z/s}; }
static inline Vec3 vec3_neg(Vec3 v) { return (Vec3){-v.x, -v.y, -v.z}; }
static inline float vec3_dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3  vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
static inline float vec3_length(Vec3 v) { return sqrtf(vec3_dot(v,v)); }
static inline Vec3  vec3_normalize(Vec3 v) { float l=vec3_length(v); return l>XSHARP_EPSILON? vec3_div(v,l):v; }
static inline Vec3  vec3_lerp(Vec3 a, Vec3 b, float t) { return vec3_add(vec3_mul(a,1-t), vec3_mul(b,t)); }
static inline Vec3  vec3_min(Vec3 a, Vec3 b) { return (Vec3){fminf(a.x,b.x), fminf(a.y,b.y), fminf(a.z,b.z)}; }
static inline Vec3  vec3_max(Vec3 a, Vec3 b) { return (Vec3){fmaxf(a.x,b.x), fmaxf(a.y,b.y), fmaxf(a.z,b.z)}; }
static inline Vec3  vec3_reflect(Vec3 v, Vec3 n) { return vec3_sub(v, vec3_mul(n, 2.0f*vec3_dot(v,n))); }
static inline Vec3  vec3_hadamard(Vec3 a, Vec3 b) { return (Vec3){a.x*b.x, a.y*b.y, a.z*b.z}; }

/* ------------------------------------------------------------------ */
/*  Vec4                                                               */
/* ------------------------------------------------------------------ */

static inline Vec4 vec4(float x, float y, float z, float w) { return (Vec4){x,y,z,w}; }
static inline Vec4 vec4_from_vec3(Vec3 v, float w) { return (Vec4){v.x,v.y,v.z,w}; }
static inline Vec3 vec4_to_vec3(Vec4 v) { return (Vec3){v.x,v.y,v.z}; }
static inline Vec4 vec4_add(Vec4 a, Vec4 b) { return (Vec4){a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w}; }
static inline Vec4 vec4_sub(Vec4 a, Vec4 b) { return (Vec4){a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w}; }
static inline Vec4 vec4_mul(Vec4 a, float s) { return (Vec4){a.x*s,a.y*s,a.z*s,a.w*s}; }
static inline Vec4 vec4_div(Vec4 a, float s) { return (Vec4){a.x/s,a.y/s,a.z/s,a.w/s}; }
static inline float vec4_dot(Vec4 a, Vec4 b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, float t) { return vec4_add(vec4_mul(a,1-t), vec4_mul(b,t)); }

/* Perspective divide */
static inline Vec3 vec4_perspective_divide(Vec4 v) {
    float inv = 1.0f / v.w;
    return (Vec3){v.x*inv, v.y*inv, v.z*inv};
}

/* ------------------------------------------------------------------ */
/*  Mat4                                                               */
/* ------------------------------------------------------------------ */

Mat4  mat4_identity(void);
Mat4  mat4_zero(void);
Mat4  mat4_translate(Vec3 t);
Mat4  mat4_scale(Vec3 s);
Mat4  mat4_rotate_x(float rad);
Mat4  mat4_rotate_y(float rad);
Mat4  mat4_rotate_z(float rad);
Mat4  mat4_rotate_axis(Vec3 axis, float rad);
Mat4  mat4_mul(Mat4 a, Mat4 b);
Vec4  mat4_mul_vec4(Mat4 m, Vec4 v);
Vec3  mat4_mul_point(Mat4 m, Vec3 p);   /* w=1 */
Vec3  mat4_mul_dir(Mat4 m, Vec3 d);     /* w=0 */
Mat4  mat4_transpose(Mat4 m);
Mat4  mat4_inverse(Mat4 m);
Mat4  mat4_perspective(float fov_rad, float aspect, float near, float far);
Mat4  mat4_ortho(float left, float right, float bottom, float top, float near, float far);
Mat4  mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);
Mat4  mat4_from_quat(Quat q);

/* ------------------------------------------------------------------ */
/*  Quaternion                                                         */
/* ------------------------------------------------------------------ */

Quat  quat_identity(void);
Quat  quat_from_euler(float pitch, float yaw, float roll);
Quat  quat_from_axis_angle(Vec3 axis, float angle_rad);
Quat  quat_mul(Quat a, Quat b);
Quat  quat_normalize(Quat q);
Quat  quat_conjugate(Quat q);
Quat  quat_slerp(Quat a, Quat b, float t);
Vec3  quat_rotate_vec3(Quat q, Vec3 v);
Mat4  quat_to_mat4(Quat q);

/* ------------------------------------------------------------------ */
/*  Intersection tests                                                 */
/* ------------------------------------------------------------------ */

bool  ray_plane_intersect(Ray ray, Plane plane, float *t_out);
bool  ray_sphere_intersect(Ray ray, Vec3 center, float radius, float *t_out);
bool  ray_triangle_intersect(Ray ray, Vec3 v0, Vec3 v1, Vec3 v2,
                             float *t_out, float *u_out, float *v_out);
bool  ray_aabb_intersect(Ray ray, AABB box, float *t_out);

/* ------------------------------------------------------------------ */
/*  AABB                                                               */
/* ------------------------------------------------------------------ */

AABB  aabb_empty(void);
AABB  aabb_expand_point(AABB box, Vec3 p);
AABB  aabb_merge(AABB a, AABB b);
AABB  aabb_transform(AABB box, Mat4 m);
bool  aabb_contains(AABB box, Vec3 p);

/* ------------------------------------------------------------------ */
/*  Frustum                                                            */
/* ------------------------------------------------------------------ */

Frustum frustum_from_vp(Mat4 vp);  /* extract from view-projection */
bool    frustum_test_point(const Frustum *f, Vec3 p);
bool    frustum_test_aabb(const Frustum *f, AABB box);
bool    frustum_test_sphere(const Frustum *f, Vec3 center, float radius);

/* ------------------------------------------------------------------ */
/*  Utility                                                            */
/* ------------------------------------------------------------------ */

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline float lerpf(float a, float b, float t) { return a + (b-a)*t; }
static inline int   mini(int a, int b) { return a < b ? a : b; }
static inline int   maxi(int a, int b) { return a > b ? a : b; }
static inline int   clampi(int v, int lo, int hi) { return v<lo?lo:(v>hi?hi:v); }

#endif /* XSHARP_MATH3D_H */
