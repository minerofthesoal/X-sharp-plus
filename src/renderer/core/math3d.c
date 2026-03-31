#include "math3d.h"
#include <float.h>
#include <stdlib.h>

/* ================================================================== */
/*  Mat4                                                               */
/* ================================================================== */

Mat4 mat4_identity(void) {
    Mat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
    return r;
}

Mat4 mat4_zero(void) {
    Mat4 r;
    memset(&r, 0, sizeof(r));
    return r;
}

Mat4 mat4_translate(Vec3 t) {
    Mat4 r = mat4_identity();
    r.m[3][0] = t.x;
    r.m[3][1] = t.y;
    r.m[3][2] = t.z;
    return r;
}

Mat4 mat4_scale(Vec3 s) {
    Mat4 r = mat4_zero();
    r.m[0][0] = s.x;
    r.m[1][1] = s.y;
    r.m[2][2] = s.z;
    r.m[3][3] = 1.0f;
    return r;
}

Mat4 mat4_rotate_x(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[1][1] = c;
    r.m[2][1] = -s;
    r.m[1][2] = s;
    r.m[2][2] = c;
    return r;
}

Mat4 mat4_rotate_y(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0][0] = c;
    r.m[2][0] = s;
    r.m[0][2] = -s;
    r.m[2][2] = c;
    return r;
}

Mat4 mat4_rotate_z(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0][0] = c;
    r.m[1][0] = -s;
    r.m[0][1] = s;
    r.m[1][1] = c;
    return r;
}

Mat4 mat4_rotate_axis(Vec3 axis, float rad) {
    axis = vec3_normalize(axis);
    float c = cosf(rad), s = sinf(rad), t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;
    Mat4 r = mat4_identity();
    r.m[0][0] = t * x * x + c;
    r.m[1][0] = t * x * y - s * z;
    r.m[2][0] = t * x * z + s * y;
    r.m[0][1] = t * x * y + s * z;
    r.m[1][1] = t * y * y + c;
    r.m[2][1] = t * y * z - s * x;
    r.m[0][2] = t * x * z - s * y;
    r.m[1][2] = t * y * z + s * x;
    r.m[2][2] = t * z * z + c;
    return r;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) {
            float sum = 0;
            for (int k = 0; k < 4; k++)
                sum += a.m[k][row] * b.m[c][k];
            r.m[c][row] = sum;
        }
    return r;
}

Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
                  m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
                  m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
                  m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w};
}

Vec3 mat4_mul_point(Mat4 m, Vec3 p) {
    Vec4 r = mat4_mul_vec4(m, vec4(p.x, p.y, p.z, 1.0f));
    return (Vec3){r.x, r.y, r.z};
}

Vec3 mat4_mul_dir(Mat4 m, Vec3 d) {
    return (Vec3){m.m[0][0] * d.x + m.m[1][0] * d.y + m.m[2][0] * d.z,
                  m.m[0][1] * d.x + m.m[1][1] * d.y + m.m[2][1] * d.z,
                  m.m[0][2] * d.x + m.m[1][2] * d.y + m.m[2][2] * d.z};
}

Mat4 mat4_transpose(Mat4 m) {
    Mat4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = m.m[j][i];
    return r;
}

/* Full 4x4 inverse using cofactors */
Mat4 mat4_inverse(Mat4 m) {
    float* s = &m.m[0][0];
    float inv[16], det;
/* Using flattened column-major indexing: element (col,row) = s[col*4+row] */
#define S(c, r) s[(c) * 4 + (r)]

    inv[0] = S(1, 1) * (S(2, 2) * S(3, 3) - S(3, 2) * S(2, 3)) -
             S(2, 1) * (S(1, 2) * S(3, 3) - S(3, 2) * S(1, 3)) +
             S(3, 1) * (S(1, 2) * S(2, 3) - S(2, 2) * S(1, 3));
    inv[1] = -S(0, 1) * (S(2, 2) * S(3, 3) - S(3, 2) * S(2, 3)) +
             S(2, 1) * (S(0, 2) * S(3, 3) - S(3, 2) * S(0, 3)) -
             S(3, 1) * (S(0, 2) * S(2, 3) - S(2, 2) * S(0, 3));
    inv[2] = S(0, 1) * (S(1, 2) * S(3, 3) - S(3, 2) * S(1, 3)) -
             S(1, 1) * (S(0, 2) * S(3, 3) - S(3, 2) * S(0, 3)) +
             S(3, 1) * (S(0, 2) * S(1, 3) - S(1, 2) * S(0, 3));
    inv[3] = -S(0, 1) * (S(1, 2) * S(2, 3) - S(2, 2) * S(1, 3)) +
             S(1, 1) * (S(0, 2) * S(2, 3) - S(2, 2) * S(0, 3)) -
             S(2, 1) * (S(0, 2) * S(1, 3) - S(1, 2) * S(0, 3));

    det = S(0, 0) * inv[0] + S(1, 0) * inv[1] + S(2, 0) * inv[2] + S(3, 0) * inv[3];
    if (fabsf(det) < XSHARP_EPSILON)
        return mat4_identity();
    det = 1.0f / det;

    inv[4] = -S(1, 0) * (S(2, 2) * S(3, 3) - S(3, 2) * S(2, 3)) +
             S(2, 0) * (S(1, 2) * S(3, 3) - S(3, 2) * S(1, 3)) -
             S(3, 0) * (S(1, 2) * S(2, 3) - S(2, 2) * S(1, 3));
    inv[5] = S(0, 0) * (S(2, 2) * S(3, 3) - S(3, 2) * S(2, 3)) -
             S(2, 0) * (S(0, 2) * S(3, 3) - S(3, 2) * S(0, 3)) +
             S(3, 0) * (S(0, 2) * S(2, 3) - S(2, 2) * S(0, 3));
    inv[6] = -S(0, 0) * (S(1, 2) * S(3, 3) - S(3, 2) * S(1, 3)) +
             S(1, 0) * (S(0, 2) * S(3, 3) - S(3, 2) * S(0, 3)) -
             S(3, 0) * (S(0, 2) * S(1, 3) - S(1, 2) * S(0, 3));
    inv[7] = S(0, 0) * (S(1, 2) * S(2, 3) - S(2, 2) * S(1, 3)) -
             S(1, 0) * (S(0, 2) * S(2, 3) - S(2, 2) * S(0, 3)) +
             S(2, 0) * (S(0, 2) * S(1, 3) - S(1, 2) * S(0, 3));

    inv[8] = S(1, 0) * (S(2, 1) * S(3, 3) - S(3, 1) * S(2, 3)) -
             S(2, 0) * (S(1, 1) * S(3, 3) - S(3, 1) * S(1, 3)) +
             S(3, 0) * (S(1, 1) * S(2, 3) - S(2, 1) * S(1, 3));
    inv[9] = -S(0, 0) * (S(2, 1) * S(3, 3) - S(3, 1) * S(2, 3)) +
             S(2, 0) * (S(0, 1) * S(3, 3) - S(3, 1) * S(0, 3)) -
             S(3, 0) * (S(0, 1) * S(2, 3) - S(2, 1) * S(0, 3));
    inv[10] = S(0, 0) * (S(1, 1) * S(3, 3) - S(3, 1) * S(1, 3)) -
              S(1, 0) * (S(0, 1) * S(3, 3) - S(3, 1) * S(0, 3)) +
              S(3, 0) * (S(0, 1) * S(1, 3) - S(1, 1) * S(0, 3));
    inv[11] = -S(0, 0) * (S(1, 1) * S(2, 3) - S(2, 1) * S(1, 3)) +
              S(1, 0) * (S(0, 1) * S(2, 3) - S(2, 1) * S(0, 3)) -
              S(2, 0) * (S(0, 1) * S(1, 3) - S(1, 1) * S(0, 3));

    inv[12] = -S(1, 0) * (S(2, 1) * S(3, 2) - S(3, 1) * S(2, 2)) +
              S(2, 0) * (S(1, 1) * S(3, 2) - S(3, 1) * S(1, 2)) -
              S(3, 0) * (S(1, 1) * S(2, 2) - S(2, 1) * S(1, 2));
    inv[13] = S(0, 0) * (S(2, 1) * S(3, 2) - S(3, 1) * S(2, 2)) -
              S(2, 0) * (S(0, 1) * S(3, 2) - S(3, 1) * S(0, 2)) +
              S(3, 0) * (S(0, 1) * S(2, 2) - S(2, 1) * S(0, 2));
    inv[14] = -S(0, 0) * (S(1, 1) * S(3, 2) - S(3, 1) * S(1, 2)) +
              S(1, 0) * (S(0, 1) * S(3, 2) - S(3, 1) * S(0, 2)) -
              S(3, 0) * (S(0, 1) * S(1, 2) - S(1, 1) * S(0, 2));
    inv[15] = S(0, 0) * (S(1, 1) * S(2, 2) - S(2, 1) * S(1, 2)) -
              S(1, 0) * (S(0, 1) * S(2, 2) - S(2, 1) * S(0, 2)) +
              S(2, 0) * (S(0, 1) * S(1, 2) - S(1, 1) * S(0, 2));
#undef S

    Mat4 r = mat4_zero();
    for (int i = 0; i < 16; i++)
        ((float*)&r)[i] = inv[i] * det;
    return r;
}

Mat4 mat4_perspective(float fov_rad, float aspect, float near, float far) {
    float t = tanf(fov_rad * 0.5f);
    Mat4 r = mat4_zero();
    r.m[0][0] = 1.0f / (aspect * t);
    r.m[1][1] = 1.0f / t;
    r.m[2][2] = -(far + near) / (far - near);
    r.m[2][3] = -1.0f;
    r.m[3][2] = -(2.0f * far * near) / (far - near);
    return r;
}

Mat4 mat4_ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 o = mat4_zero();
    o.m[0][0] = 2.0f / (r - l);
    o.m[1][1] = 2.0f / (t - b);
    o.m[2][2] = -2.0f / (f - n);
    o.m[3][0] = -(r + l) / (r - l);
    o.m[3][1] = -(t + b) / (t - b);
    o.m[3][2] = -(f + n) / (f - n);
    o.m[3][3] = 1.0f;
    return o;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    Mat4 r = mat4_identity();
    r.m[0][0] = s.x;
    r.m[1][0] = s.y;
    r.m[2][0] = s.z;
    r.m[0][1] = u.x;
    r.m[1][1] = u.y;
    r.m[2][1] = u.z;
    r.m[0][2] = -f.x;
    r.m[1][2] = -f.y;
    r.m[2][2] = -f.z;
    r.m[3][0] = -vec3_dot(s, eye);
    r.m[3][1] = -vec3_dot(u, eye);
    r.m[3][2] = vec3_dot(f, eye);
    return r;
}

Mat4 mat4_from_quat(Quat q) {
    return quat_to_mat4(q);
}

/* ================================================================== */
/*  Quaternion                                                         */
/* ================================================================== */

Quat quat_identity(void) {
    return (Quat){0, 0, 0, 1};
}

Quat quat_from_euler(float pitch, float yaw, float roll) {
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
    float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    return (Quat){sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
                  cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy};
}

Quat quat_from_axis_angle(Vec3 axis, float angle) {
    axis = vec3_normalize(axis);
    float half = angle * 0.5f;
    float s = sinf(half);
    return (Quat){axis.x * s, axis.y * s, axis.z * s, cosf(half)};
}

Quat quat_mul(Quat a, Quat b) {
    return (Quat){a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                  a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                  a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                  a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

Quat quat_normalize(Quat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < XSHARP_EPSILON)
        return quat_identity();
    float inv = 1.0f / len;
    return (Quat){q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

Quat quat_conjugate(Quat q) {
    return (Quat){-q.x, -q.y, -q.z, q.w};
}

Quat quat_slerp(Quat a, Quat b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0f) {
        b = (Quat){-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }
    if (dot > 0.9995f) {
        Quat r = {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z),
                  a.w + t * (b.w - a.w)};
        return quat_normalize(r);
    }
    float theta = acosf(dot);
    float sn = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sn;
    float wb = sinf(t * theta) / sn;
    return (Quat){wa * a.x + wb * b.x, wa * a.y + wb * b.y, wa * a.z + wb * b.z,
                  wa * a.w + wb * b.w};
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    Vec3 u = {q.x, q.y, q.z};
    float s = q.w;
    return vec3_add(
        vec3_add(vec3_mul(u, 2.0f * vec3_dot(u, v)), vec3_mul(v, s * s - vec3_dot(u, u))),
        vec3_mul(vec3_cross(u, v), 2.0f * s));
}

Mat4 quat_to_mat4(Quat q) {
    q = quat_normalize(q);
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    Mat4 r = mat4_identity();
    r.m[0][0] = 1 - 2 * (yy + zz);
    r.m[1][0] = 2 * (xy - wz);
    r.m[2][0] = 2 * (xz + wy);
    r.m[0][1] = 2 * (xy + wz);
    r.m[1][1] = 1 - 2 * (xx + zz);
    r.m[2][1] = 2 * (yz - wx);
    r.m[0][2] = 2 * (xz - wy);
    r.m[1][2] = 2 * (yz + wx);
    r.m[2][2] = 1 - 2 * (xx + yy);
    return r;
}

/* ================================================================== */
/*  Intersection tests                                                 */
/* ================================================================== */

bool ray_plane_intersect(Ray ray, Plane plane, float* t_out) {
    float denom = vec3_dot(plane.normal, ray.dir);
    if (fabsf(denom) < XSHARP_EPSILON)
        return false;
    float t = -(vec3_dot(plane.normal, ray.origin) + plane.d) / denom;
    if (t < 0)
        return false;
    if (t_out)
        *t_out = t;
    return true;
}

bool ray_sphere_intersect(Ray ray, Vec3 center, float radius, float* t_out) {
    Vec3 oc = vec3_sub(ray.origin, center);
    float a = vec3_dot(ray.dir, ray.dir);
    float b = 2.0f * vec3_dot(oc, ray.dir);
    float c = vec3_dot(oc, oc) - radius * radius;
    float disc = b * b - 4 * a * c;
    if (disc < 0)
        return false;
    float sq = sqrtf(disc);
    float t1 = (-b - sq) / (2 * a);
    float t2 = (-b + sq) / (2 * a);
    float t = (t1 >= 0) ? t1 : t2;
    if (t < 0)
        return false;
    if (t_out)
        *t_out = t;
    return true;
}

/* Moller-Trumbore */
bool ray_triangle_intersect(Ray ray, Vec3 v0, Vec3 v1, Vec3 v2, float* t_out, float* u_out,
                            float* v_out) {
    Vec3 e1 = vec3_sub(v1, v0);
    Vec3 e2 = vec3_sub(v2, v0);
    Vec3 h = vec3_cross(ray.dir, e2);
    float a = vec3_dot(e1, h);
    if (fabsf(a) < XSHARP_EPSILON)
        return false;
    float f = 1.0f / a;
    Vec3 s = vec3_sub(ray.origin, v0);
    float u = f * vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    Vec3 q = vec3_cross(s, e1);
    float v = f * vec3_dot(ray.dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t = f * vec3_dot(e2, q);
    if (t < XSHARP_EPSILON)
        return false;
    if (t_out)
        *t_out = t;
    if (u_out)
        *u_out = u;
    if (v_out)
        *v_out = v;
    return true;
}

bool ray_aabb_intersect(Ray ray, AABB box, float* t_out) {
    float tmin = -FLT_MAX, tmax = FLT_MAX;
    float orig[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    float dir[3] = {ray.dir.x, ray.dir.y, ray.dir.z};
    float bmin[3] = {box.min.x, box.min.y, box.min.z};
    float bmax[3] = {box.max.x, box.max.y, box.max.z};
    for (int i = 0; i < 3; i++) {
        if (fabsf(dir[i]) < XSHARP_EPSILON) {
            if (orig[i] < bmin[i] || orig[i] > bmax[i])
                return false;
        } else {
            float inv = 1.0f / dir[i];
            float t1 = (bmin[i] - orig[i]) * inv;
            float t2 = (bmax[i] - orig[i]) * inv;
            if (t1 > t2) {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tmin)
                tmin = t1;
            if (t2 < tmax)
                tmax = t2;
            if (tmin > tmax)
                return false;
        }
    }
    if (tmax < 0)
        return false;
    if (t_out)
        *t_out = tmin >= 0 ? tmin : tmax;
    return true;
}

/* ================================================================== */
/*  AABB                                                               */
/* ================================================================== */

AABB aabb_empty(void) {
    return (AABB){{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
}

AABB aabb_expand_point(AABB box, Vec3 p) {
    box.min = vec3_min(box.min, p);
    box.max = vec3_max(box.max, p);
    return box;
}

AABB aabb_merge(AABB a, AABB b) {
    return (AABB){vec3_min(a.min, b.min), vec3_max(a.max, b.max)};
}

AABB aabb_transform(AABB box, Mat4 m) {
    Vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z},
    };
    AABB result = aabb_empty();
    for (int i = 0; i < 8; i++)
        result = aabb_expand_point(result, mat4_mul_point(m, corners[i]));
    return result;
}

bool aabb_contains(AABB box, Vec3 p) {
    return p.x >= box.min.x && p.x <= box.max.x && p.y >= box.min.y && p.y <= box.max.y &&
           p.z >= box.min.z && p.z <= box.max.z;
}

/* ================================================================== */
/*  Frustum                                                            */
/* ================================================================== */

Frustum frustum_from_vp(Mat4 vp) {
    Frustum f;
    float* m = &vp.m[0][0];
/* Column-major: element(col,row) = m[col*4+row] */
#define M(c, r) m[(c) * 4 + (r)]
    /* Left */
    f.planes[0].normal = (Vec3){M(0, 3) + M(0, 0), M(1, 3) + M(1, 0), M(2, 3) + M(2, 0)};
    f.planes[0].d = M(3, 3) + M(3, 0);
    /* Right */
    f.planes[1].normal = (Vec3){M(0, 3) - M(0, 0), M(1, 3) - M(1, 0), M(2, 3) - M(2, 0)};
    f.planes[1].d = M(3, 3) - M(3, 0);
    /* Bottom */
    f.planes[2].normal = (Vec3){M(0, 3) + M(0, 1), M(1, 3) + M(1, 1), M(2, 3) + M(2, 1)};
    f.planes[2].d = M(3, 3) + M(3, 1);
    /* Top */
    f.planes[3].normal = (Vec3){M(0, 3) - M(0, 1), M(1, 3) - M(1, 1), M(2, 3) - M(2, 1)};
    f.planes[3].d = M(3, 3) - M(3, 1);
    /* Near */
    f.planes[4].normal = (Vec3){M(0, 3) + M(0, 2), M(1, 3) + M(1, 2), M(2, 3) + M(2, 2)};
    f.planes[4].d = M(3, 3) + M(3, 2);
    /* Far */
    f.planes[5].normal = (Vec3){M(0, 3) - M(0, 2), M(1, 3) - M(1, 2), M(2, 3) - M(2, 2)};
    f.planes[5].d = M(3, 3) - M(3, 2);
#undef M
    /* Normalize all planes */
    for (int i = 0; i < 6; i++) {
        float len = vec3_length(f.planes[i].normal);
        if (len > XSHARP_EPSILON) {
            f.planes[i].normal = vec3_div(f.planes[i].normal, len);
            f.planes[i].d /= len;
        }
    }
    return f;
}

bool frustum_test_point(const Frustum* f, Vec3 p) {
    for (int i = 0; i < 6; i++)
        if (vec3_dot(f->planes[i].normal, p) + f->planes[i].d < 0)
            return false;
    return true;
}

bool frustum_test_aabb(const Frustum* f, AABB box) {
    for (int i = 0; i < 6; i++) {
        Vec3 n = f->planes[i].normal;
        /* Find the positive vertex (the one farthest along the plane normal) */
        Vec3 pv = {n.x >= 0 ? box.max.x : box.min.x, n.y >= 0 ? box.max.y : box.min.y,
                   n.z >= 0 ? box.max.z : box.min.z};
        if (vec3_dot(n, pv) + f->planes[i].d < 0)
            return false;
    }
    return true;
}

bool frustum_test_sphere(const Frustum* f, Vec3 center, float radius) {
    for (int i = 0; i < 6; i++)
        if (vec3_dot(f->planes[i].normal, center) + f->planes[i].d < -radius)
            return false;
    return true;
}
