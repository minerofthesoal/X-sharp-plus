#include "mesh.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

/* ================================================================== */
/*  Create / Destroy                                                   */
/* ================================================================== */

Mesh *mesh_create(const Vertex *verts, uint32_t vert_count,
                  const uint32_t *indices, uint32_t idx_count) {
    Mesh *m = calloc(1, sizeof(Mesh));
    if (!m) return NULL;

    m->vertex_count = vert_count;
    m->index_count  = idx_count;
    m->vertices = malloc(sizeof(Vertex) * vert_count);
    m->indices  = malloc(sizeof(uint32_t) * idx_count);
    if (!m->vertices || !m->indices) {
        free(m->vertices); free(m->indices); free(m);
        return NULL;
    }
    memcpy(m->vertices, verts, sizeof(Vertex) * vert_count);
    memcpy(m->indices, indices, sizeof(uint32_t) * idx_count);
    mesh_compute_bounds(m);
    return m;
}

Mesh *mesh_create_empty(uint32_t vert_cap, uint32_t idx_cap) {
    Mesh *m = calloc(1, sizeof(Mesh));
    if (!m) return NULL;
    m->vertices = calloc(vert_cap, sizeof(Vertex));
    m->indices  = calloc(idx_cap, sizeof(uint32_t));
    if (!m->vertices || !m->indices) {
        free(m->vertices); free(m->indices); free(m);
        return NULL;
    }
    m->bounds = aabb_empty();
    return m;
}

void mesh_destroy(Mesh *m) {
    if (!m) return;
    free(m->vertices);
    free(m->indices);
    free(m);
}

void mesh_compute_bounds(Mesh *m) {
    if (!m || m->vertex_count == 0) { m->bounds = aabb_empty(); return; }
    AABB b = aabb_empty();
    for (uint32_t i = 0; i < m->vertex_count; i++)
        b = aabb_expand_point(b, m->vertices[i].position);
    m->bounds = b;
}

void mesh_compute_normals(Mesh *m) {
    if (!m) return;
    /* Zero all normals */
    for (uint32_t i = 0; i < m->vertex_count; i++)
        m->vertices[i].normal = vec3(0, 0, 0);

    /* Accumulate face normals */
    for (uint32_t i = 0; i + 2 < m->index_count; i += 3) {
        uint32_t i0 = m->indices[i], i1 = m->indices[i+1], i2 = m->indices[i+2];
        Vec3 e1 = vec3_sub(m->vertices[i1].position, m->vertices[i0].position);
        Vec3 e2 = vec3_sub(m->vertices[i2].position, m->vertices[i0].position);
        Vec3 fn = vec3_cross(e1, e2);
        m->vertices[i0].normal = vec3_add(m->vertices[i0].normal, fn);
        m->vertices[i1].normal = vec3_add(m->vertices[i1].normal, fn);
        m->vertices[i2].normal = vec3_add(m->vertices[i2].normal, fn);
    }

    /* Normalize */
    for (uint32_t i = 0; i < m->vertex_count; i++)
        m->vertices[i].normal = vec3_normalize(m->vertices[i].normal);
}

void mesh_transform(Mesh *m, Mat4 mat) {
    if (!m) return;
    /* Normal matrix: transpose of inverse of upper-left 3x3 */
    Mat4 nmat = mat4_transpose(mat4_inverse(mat));
    for (uint32_t i = 0; i < m->vertex_count; i++) {
        m->vertices[i].position = mat4_mul_point(mat, m->vertices[i].position);
        m->vertices[i].normal   = vec3_normalize(mat4_mul_dir(nmat, m->vertices[i].normal));
    }
    mesh_compute_bounds(m);
}

/* ================================================================== */
/*  Primitive: Cube                                                    */
/* ================================================================== */

Mesh *mesh_create_cube(float size) {
    float h = size * 0.5f;
    /* 24 vertices (4 per face, unique normals) */
    Vertex verts[24];
    uint32_t indices[36];
    Vec4 white = {1,1,1,1};

    int vi = 0, ii = 0;

    /* Helper macro: add a face quad */
    #define FACE(p0,p1,p2,p3,n) do { \
        int base = vi; \
        verts[vi++] = (Vertex){p0, n, {0,0}, white}; \
        verts[vi++] = (Vertex){p1, n, {1,0}, white}; \
        verts[vi++] = (Vertex){p2, n, {1,1}, white}; \
        verts[vi++] = (Vertex){p3, n, {0,1}, white}; \
        indices[ii++] = base; indices[ii++] = base+1; indices[ii++] = base+2; \
        indices[ii++] = base; indices[ii++] = base+2; indices[ii++] = base+3; \
    } while(0)

    /* +Z face */
    FACE(((Vec3){-h,-h, h}), ((Vec3){ h,-h, h}), ((Vec3){ h, h, h}), ((Vec3){-h, h, h}), ((Vec3){0,0,1}));
    /* -Z face */
    FACE(((Vec3){ h,-h,-h}), ((Vec3){-h,-h,-h}), ((Vec3){-h, h,-h}), ((Vec3){ h, h,-h}), ((Vec3){0,0,-1}));
    /* +X face */
    FACE(((Vec3){ h,-h, h}), ((Vec3){ h,-h,-h}), ((Vec3){ h, h,-h}), ((Vec3){ h, h, h}), ((Vec3){1,0,0}));
    /* -X face */
    FACE(((Vec3){-h,-h,-h}), ((Vec3){-h,-h, h}), ((Vec3){-h, h, h}), ((Vec3){-h, h,-h}), ((Vec3){-1,0,0}));
    /* +Y face */
    FACE(((Vec3){-h, h, h}), ((Vec3){ h, h, h}), ((Vec3){ h, h,-h}), ((Vec3){-h, h,-h}), ((Vec3){0,1,0}));
    /* -Y face */
    FACE(((Vec3){-h,-h,-h}), ((Vec3){ h,-h,-h}), ((Vec3){ h,-h, h}), ((Vec3){-h,-h, h}), ((Vec3){0,-1,0}));

    #undef FACE

    return mesh_create(verts, 24, indices, 36);
}

/* ================================================================== */
/*  Primitive: Plane                                                   */
/* ================================================================== */

Mesh *mesh_create_plane(float width, float depth, int sx, int sz) {
    if (sx < 1) sx = 1;
    if (sz < 1) sz = 1;
    int vx = sx + 1, vz = sz + 1;
    uint32_t vc = vx * vz;
    uint32_t ic = sx * sz * 6;

    Vertex *verts = calloc(vc, sizeof(Vertex));
    uint32_t *indices = calloc(ic, sizeof(uint32_t));
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    Vec3 normal = {0, 1, 0};
    Vec4 white  = {1, 1, 1, 1};

    for (int z = 0; z < vz; z++) {
        for (int x = 0; x < vx; x++) {
            int idx = z * vx + x;
            float u = (float)x / sx;
            float v = (float)z / sz;
            verts[idx].position = vec3(u * width - width * 0.5f, 0, v * depth - depth * 0.5f);
            verts[idx].normal   = normal;
            verts[idx].uv       = vec2(u, v);
            verts[idx].color    = white;
        }
    }

    int ii = 0;
    for (int z = 0; z < sz; z++) {
        for (int x = 0; x < sx; x++) {
            int bl = z * vx + x;
            int br = bl + 1;
            int tl = bl + vx;
            int tr = tl + 1;
            indices[ii++] = bl; indices[ii++] = br; indices[ii++] = tr;
            indices[ii++] = bl; indices[ii++] = tr; indices[ii++] = tl;
        }
    }

    Mesh *m = mesh_create(verts, vc, indices, ic);
    free(verts); free(indices);
    return m;
}

/* ================================================================== */
/*  Primitive: Sphere (UV sphere)                                      */
/* ================================================================== */

Mesh *mesh_create_sphere(float radius, int slices, int stacks) {
    if (slices < 3) slices = 3;
    if (stacks < 2) stacks = 2;

    uint32_t vc = (slices + 1) * (stacks + 1);
    uint32_t ic = slices * stacks * 6;
    Vertex *verts = calloc(vc, sizeof(Vertex));
    uint32_t *indices = calloc(ic, sizeof(uint32_t));
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    Vec4 white = {1,1,1,1};
    int vi = 0;
    for (int st = 0; st <= stacks; st++) {
        float v = (float)st / stacks;
        float phi = v * (float)M_PI;
        for (int sl = 0; sl <= slices; sl++) {
            float u = (float)sl / slices;
            float theta = u * 2.0f * (float)M_PI;
            Vec3 n = vec3(sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta));
            verts[vi].position = vec3_mul(n, radius);
            verts[vi].normal   = n;
            verts[vi].uv       = vec2(u, v);
            verts[vi].color    = white;
            vi++;
        }
    }

    int ii = 0;
    for (int st = 0; st < stacks; st++) {
        for (int sl = 0; sl < slices; sl++) {
            int a = st * (slices + 1) + sl;
            int b = a + slices + 1;
            indices[ii++] = a;   indices[ii++] = b;   indices[ii++] = a+1;
            indices[ii++] = a+1; indices[ii++] = b;   indices[ii++] = b+1;
        }
    }

    Mesh *m = mesh_create(verts, vc, indices, ic);
    free(verts); free(indices);
    return m;
}

/* ================================================================== */
/*  Primitive: Cylinder                                                */
/* ================================================================== */

Mesh *mesh_create_cylinder(float radius, float height, int segments) {
    if (segments < 3) segments = 3;
    float hh = height * 0.5f;

    /* Side: 2 rings of (segments+1) verts + 2 center verts for caps */
    uint32_t side_verts = (segments + 1) * 2;
    uint32_t cap_verts  = (segments + 1) * 2 + 2; /* top ring + bottom ring + 2 centers */
    uint32_t vc = side_verts + cap_verts;
    uint32_t side_idx = segments * 6;
    uint32_t cap_idx  = segments * 3 * 2;
    uint32_t ic = side_idx + cap_idx;

    Vertex *verts = calloc(vc, sizeof(Vertex));
    uint32_t *indices = calloc(ic, sizeof(uint32_t));
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    Vec4 white = {1,1,1,1};
    int vi = 0, ii = 0;

    /* Side vertices */
    for (int i = 0; i <= segments; i++) {
        float u = (float)i / segments;
        float theta = u * 2.0f * (float)M_PI;
        float c = cosf(theta), s = sinf(theta);
        Vec3 n = vec3(c, 0, s);
        /* Bottom */
        verts[vi].position = vec3(c * radius, -hh, s * radius);
        verts[vi].normal = n; verts[vi].uv = vec2(u, 0); verts[vi].color = white;
        vi++;
        /* Top */
        verts[vi].position = vec3(c * radius,  hh, s * radius);
        verts[vi].normal = n; verts[vi].uv = vec2(u, 1); verts[vi].color = white;
        vi++;
    }
    /* Side indices */
    for (int i = 0; i < segments; i++) {
        int b = i * 2, t = b + 1;
        indices[ii++] = b;   indices[ii++] = b+2; indices[ii++] = t;
        indices[ii++] = t;   indices[ii++] = b+2; indices[ii++] = t+2;
    }

    /* Top cap */
    int top_center = vi;
    verts[vi].position = vec3(0, hh, 0);
    verts[vi].normal = vec3(0, 1, 0); verts[vi].uv = vec2(0.5f, 0.5f); verts[vi].color = white;
    vi++;
    int top_base = vi;
    for (int i = 0; i <= segments; i++) {
        float u = (float)i / segments;
        float theta = u * 2.0f * (float)M_PI;
        verts[vi].position = vec3(cosf(theta) * radius, hh, sinf(theta) * radius);
        verts[vi].normal = vec3(0, 1, 0);
        verts[vi].uv = vec2(cosf(theta)*0.5f+0.5f, sinf(theta)*0.5f+0.5f);
        verts[vi].color = white;
        vi++;
    }
    for (int i = 0; i < segments; i++) {
        indices[ii++] = top_center; indices[ii++] = top_base + i; indices[ii++] = top_base + i + 1;
    }

    /* Bottom cap */
    int bot_center = vi;
    verts[vi].position = vec3(0, -hh, 0);
    verts[vi].normal = vec3(0, -1, 0); verts[vi].uv = vec2(0.5f, 0.5f); verts[vi].color = white;
    vi++;
    int bot_base = vi;
    for (int i = 0; i <= segments; i++) {
        float u = (float)i / segments;
        float theta = u * 2.0f * (float)M_PI;
        verts[vi].position = vec3(cosf(theta) * radius, -hh, sinf(theta) * radius);
        verts[vi].normal = vec3(0, -1, 0);
        verts[vi].uv = vec2(cosf(theta)*0.5f+0.5f, sinf(theta)*0.5f+0.5f);
        verts[vi].color = white;
        vi++;
    }
    for (int i = 0; i < segments; i++) {
        indices[ii++] = bot_center; indices[ii++] = bot_base + i + 1; indices[ii++] = bot_base + i;
    }

    Mesh *m = mesh_create(verts, vi, indices, ii);
    free(verts); free(indices);
    return m;
}

/* ================================================================== */
/*  Primitive: Cone                                                    */
/* ================================================================== */

Mesh *mesh_create_cone(float radius, float height, int segments) {
    if (segments < 3) segments = 3;

    /* Apex + base ring + base center */
    uint32_t vc = 1 + (segments + 1) * 2 + 1 + (segments + 1);
    uint32_t ic = segments * 3 + segments * 3; /* side + base */

    Vertex *verts = calloc(vc, sizeof(Vertex));
    uint32_t *indices = calloc(ic, sizeof(uint32_t));
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    Vec4 white = {1,1,1,1};
    int vi = 0, ii = 0;

    /* Apex */
    int apex = vi;
    verts[vi].position = vec3(0, height, 0);
    verts[vi].normal = vec3(0, 1, 0);
    verts[vi].uv = vec2(0.5f, 1.0f);
    verts[vi].color = white;
    vi++;

    /* Side base ring */
    float slope = radius / height;
    int side_base = vi;
    for (int i = 0; i <= segments; i++) {
        float u = (float)i / segments;
        float theta = u * 2.0f * (float)M_PI;
        float c = cosf(theta), s = sinf(theta);
        /* Normal for cone side */
        Vec3 n = vec3_normalize(vec3(c, slope, s));
        verts[vi].position = vec3(c * radius, 0, s * radius);
        verts[vi].normal = n;
        verts[vi].uv = vec2(u, 0);
        verts[vi].color = white;
        vi++;
    }
    /* Side triangles */
    for (int i = 0; i < segments; i++) {
        indices[ii++] = apex;
        indices[ii++] = side_base + i;
        indices[ii++] = side_base + i + 1;
    }

    /* Bottom cap */
    int bot_center = vi;
    verts[vi].position = vec3(0, 0, 0);
    verts[vi].normal = vec3(0, -1, 0);
    verts[vi].uv = vec2(0.5f, 0.5f);
    verts[vi].color = white;
    vi++;
    int bot_ring = vi;
    for (int i = 0; i <= segments; i++) {
        float u = (float)i / segments;
        float theta = u * 2.0f * (float)M_PI;
        verts[vi].position = vec3(cosf(theta)*radius, 0, sinf(theta)*radius);
        verts[vi].normal = vec3(0, -1, 0);
        verts[vi].uv = vec2(cosf(theta)*0.5f+0.5f, sinf(theta)*0.5f+0.5f);
        verts[vi].color = white;
        vi++;
    }
    for (int i = 0; i < segments; i++) {
        indices[ii++] = bot_center;
        indices[ii++] = bot_ring + i + 1;
        indices[ii++] = bot_ring + i;
    }

    Mesh *m = mesh_create(verts, vi, indices, ii);
    free(verts); free(indices);
    return m;
}

/* ================================================================== */
/*  Primitive: Torus                                                   */
/* ================================================================== */

Mesh *mesh_create_torus(float major_r, float minor_r, int major_seg, int minor_seg) {
    if (major_seg < 3) major_seg = 3;
    if (minor_seg < 3) minor_seg = 3;

    uint32_t vc = (major_seg + 1) * (minor_seg + 1);
    uint32_t ic = major_seg * minor_seg * 6;

    Vertex *verts = calloc(vc, sizeof(Vertex));
    uint32_t *indices = calloc(ic, sizeof(uint32_t));
    if (!verts || !indices) { free(verts); free(indices); return NULL; }

    Vec4 white = {1,1,1,1};
    int vi = 0;

    for (int i = 0; i <= major_seg; i++) {
        float u = (float)i / major_seg;
        float theta = u * 2.0f * (float)M_PI;
        float ct = cosf(theta), st = sinf(theta);
        for (int j = 0; j <= minor_seg; j++) {
            float v = (float)j / minor_seg;
            float phi = v * 2.0f * (float)M_PI;
            float cp = cosf(phi), sp = sinf(phi);

            float x = (major_r + minor_r * cp) * ct;
            float y = minor_r * sp;
            float z = (major_r + minor_r * cp) * st;

            /* Normal: direction from tube center to surface point */
            Vec3 center = vec3(major_r * ct, 0, major_r * st);
            Vec3 pos = vec3(x, y, z);
            Vec3 n = vec3_normalize(vec3_sub(pos, center));

            verts[vi].position = pos;
            verts[vi].normal   = n;
            verts[vi].uv       = vec2(u, v);
            verts[vi].color    = white;
            vi++;
        }
    }

    int ii = 0;
    for (int i = 0; i < major_seg; i++) {
        for (int j = 0; j < minor_seg; j++) {
            int a = i * (minor_seg + 1) + j;
            int b = a + minor_seg + 1;
            indices[ii++] = a;   indices[ii++] = b;   indices[ii++] = a+1;
            indices[ii++] = a+1; indices[ii++] = b;   indices[ii++] = b+1;
        }
    }

    Mesh *m = mesh_create(verts, vi, indices, ii);
    free(verts); free(indices);
    return m;
}

/* ================================================================== */
/*  OBJ loading                                                        */
/* ================================================================== */

/* Simple dynamic array for OBJ parsing */
typedef struct { float *data; int count, cap; } FloatArray;
typedef struct { uint32_t *data; int count, cap; } UintArray;

static void fa_init(FloatArray *a) { a->data = NULL; a->count = 0; a->cap = 0; }
static void fa_push(FloatArray *a, float v) {
    if (a->count >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 256;
        a->data = realloc(a->data, sizeof(float) * a->cap);
    }
    a->data[a->count++] = v;
}
static void ua_init(UintArray *a) { a->data = NULL; a->count = 0; a->cap = 0; }
static void ua_push(UintArray *a, uint32_t v) {
    if (a->count >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 256;
        a->data = realloc(a->data, sizeof(uint32_t) * a->cap);
    }
    a->data[a->count++] = v;
}

/* Hash a v/vt/vn triplet to deduplicate vertices */
typedef struct { int v, vt, vn; uint32_t out_idx; } ObjVertKey;
typedef struct { ObjVertKey *entries; int cap; } ObjVertMap;

static uint32_t obj_hash(int v, int vt, int vn, int cap) {
    uint32_t h = (uint32_t)(v * 73856093 ^ vt * 19349663 ^ vn * 83492791);
    return h % (uint32_t)cap;
}

static int obj_map_find_or_insert(ObjVertMap *map, int v, int vt, int vn, uint32_t next_idx) {
    uint32_t h = obj_hash(v, vt, vn, map->cap);
    for (int i = 0; i < map->cap; i++) {
        uint32_t idx = (h + i) % (uint32_t)map->cap;
        ObjVertKey *e = &map->entries[idx];
        if (e->v == 0 && e->vt == 0 && e->vn == 0) {
            /* Empty slot: insert */
            e->v = v; e->vt = vt; e->vn = vn; e->out_idx = next_idx;
            return -1; /* new entry */
        }
        if (e->v == v && e->vt == vt && e->vn == vn)
            return (int)e->out_idx; /* found */
    }
    return -1;
}

Mesh *mesh_load_obj(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    FloatArray positions, normals, texcoords;
    fa_init(&positions); fa_init(&normals); fa_init(&texcoords);

    /* Output arrays */
    int vert_count = 0, vert_cap = 1024;
    Vertex *out_verts = malloc(sizeof(Vertex) * vert_cap);
    UintArray out_indices;
    ua_init(&out_indices);

    /* Vertex dedup map */
    int map_cap = 4096;
    ObjVertMap map;
    map.cap = map_cap;
    map.entries = calloc(map_cap, sizeof(ObjVertKey));

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            if (sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3) {
                fa_push(&positions, x); fa_push(&positions, y); fa_push(&positions, z);
            }
        } else if (line[0] == 'v' && line[1] == 'n') {
            float x, y, z;
            if (sscanf(line + 3, "%f %f %f", &x, &y, &z) == 3) {
                fa_push(&normals, x); fa_push(&normals, y); fa_push(&normals, z);
            }
        } else if (line[0] == 'v' && line[1] == 't') {
            float u, v;
            if (sscanf(line + 3, "%f %f", &u, &v) >= 1) {
                fa_push(&texcoords, u); fa_push(&texcoords, v);
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            /* Parse face - supports v, v/vt, v/vt/vn, v//vn */
            int face_verts[16];
            int face_count = 0;

            char *p = line + 2;
            while (*p && face_count < 16) {
                int vi = 0, ti = 0, ni = 0;
                /* Try different formats */
                if (sscanf(p, "%d/%d/%d", &vi, &ti, &ni) == 3) {
                    /* v/vt/vn */
                } else if (sscanf(p, "%d//%d", &vi, &ni) == 2) {
                    ti = 0;
                } else if (sscanf(p, "%d/%d", &vi, &ti) == 2) {
                    ni = 0;
                } else if (sscanf(p, "%d", &vi) == 1) {
                    ti = 0; ni = 0;
                } else {
                    break;
                }

                /* Resolve negative indices */
                if (vi < 0) vi = positions.count / 3 + vi + 1;
                if (ti < 0) ti = texcoords.count / 2 + ti + 1;
                if (ni < 0) ni = normals.count / 3 + ni + 1;

                /* Grow map if needed */
                if (vert_count > map.cap / 2) {
                    int new_cap = map.cap * 2;
                    ObjVertKey *new_entries = calloc(new_cap, sizeof(ObjVertKey));
                    /* Rehash */
                    for (int k = 0; k < map.cap; k++) {
                        ObjVertKey *e = &map.entries[k];
                        if (e->v != 0 || e->vt != 0 || e->vn != 0) {
                            uint32_t h = obj_hash(e->v, e->vt, e->vn, new_cap);
                            for (int j = 0; j < new_cap; j++) {
                                uint32_t idx2 = (h + j) % (uint32_t)new_cap;
                                if (new_entries[idx2].v == 0 && new_entries[idx2].vt == 0 && new_entries[idx2].vn == 0) {
                                    new_entries[idx2] = *e;
                                    break;
                                }
                            }
                        }
                    }
                    free(map.entries);
                    map.entries = new_entries;
                    map.cap = new_cap;
                }

                int existing = obj_map_find_or_insert(&map, vi, ti, ni, (uint32_t)vert_count);
                if (existing >= 0) {
                    face_verts[face_count++] = existing;
                } else {
                    /* Create new vertex */
                    if (vert_count >= vert_cap) {
                        vert_cap *= 2;
                        out_verts = realloc(out_verts, sizeof(Vertex) * vert_cap);
                    }
                    Vertex v;
                    memset(&v, 0, sizeof(v));
                    v.color = (Vec4){1,1,1,1};

                    if (vi >= 1 && (vi-1)*3+2 < positions.count) {
                        v.position = vec3(positions.data[(vi-1)*3],
                                          positions.data[(vi-1)*3+1],
                                          positions.data[(vi-1)*3+2]);
                    }
                    if (ti >= 1 && (ti-1)*2+1 < texcoords.count) {
                        v.uv = vec2(texcoords.data[(ti-1)*2],
                                    texcoords.data[(ti-1)*2+1]);
                    }
                    if (ni >= 1 && (ni-1)*3+2 < normals.count) {
                        v.normal = vec3(normals.data[(ni-1)*3],
                                        normals.data[(ni-1)*3+1],
                                        normals.data[(ni-1)*3+2]);
                    }

                    out_verts[vert_count] = v;
                    face_verts[face_count++] = vert_count;
                    vert_count++;
                }

                /* Skip to next token */
                while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
                while (*p == ' ' || *p == '\t') p++;
            }

            /* Triangulate face (fan) */
            for (int i = 2; i < face_count; i++) {
                ua_push(&out_indices, (uint32_t)face_verts[0]);
                ua_push(&out_indices, (uint32_t)face_verts[i-1]);
                ua_push(&out_indices, (uint32_t)face_verts[i]);
            }
        }
    }
    fclose(fp);

    Mesh *m = NULL;
    if (vert_count > 0 && out_indices.count > 0) {
        m = mesh_create(out_verts, vert_count, out_indices.data, out_indices.count);
        /* If no normals were loaded, compute them */
        if (normals.count == 0 && m)
            mesh_compute_normals(m);
    }

    free(positions.data);
    free(normals.data);
    free(texcoords.data);
    free(out_verts);
    free(out_indices.data);
    free(map.entries);
    return m;
}
