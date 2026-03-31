#include "shader.h"
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/*  Shader program management                                          */
/* ================================================================== */

ShaderProgram* shader_create(const char* name, VertexShaderFn vs, FragmentShaderFn fs,
                             int varying_count) {
    ShaderProgram* prog = calloc(1, sizeof(ShaderProgram));
    if (!prog)
        return NULL;
    if (name)
        strncpy(prog->name, name, sizeof(prog->name) - 1);
    prog->vertex_fn = vs;
    prog->fragment_fn = fs;
    prog->varying_count = varying_count;
    prog->uniform_count = 0;
    return prog;
}

void shader_destroy(ShaderProgram* prog) {
    free(prog);
}

/* Find or create a uniform slot by name */
static ShaderUniform* find_or_create_uniform(ShaderProgram* prog, const char* name) {
    for (int i = 0; i < prog->uniform_count; i++) {
        if (strcmp(prog->uniforms[i].name, name) == 0)
            return &prog->uniforms[i];
    }
    if (prog->uniform_count >= SHADER_MAX_UNIFORMS)
        return NULL;
    ShaderUniform* u = &prog->uniforms[prog->uniform_count++];
    memset(u, 0, sizeof(*u));
    strncpy(u->name, name, sizeof(u->name) - 1);
    return u;
}

void shader_set_float(ShaderProgram* prog, const char* name, float v) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_FLOAT;
        u->data.f = v;
    }
}

void shader_set_int(ShaderProgram* prog, const char* name, int v) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_INT;
        u->data.i = v;
    }
}

void shader_set_vec3(ShaderProgram* prog, const char* name, Vec3 v) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_VEC3;
        u->data.v3 = v;
    }
}

void shader_set_vec4(ShaderProgram* prog, const char* name, Vec4 v) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_VEC4;
        u->data.v4 = v;
    }
}

void shader_set_mat4(ShaderProgram* prog, const char* name, Mat4 v) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_MAT4;
        u->data.m4 = v;
    }
}

void shader_set_texture(ShaderProgram* prog, const char* name, const XSharpTexture* tex) {
    if (!prog)
        return;
    ShaderUniform* u = find_or_create_uniform(prog, name);
    if (u) {
        u->type = UNIFORM_TEXTURE;
        u->data.tex = tex;
    }
}

const ShaderUniform* shader_get_uniform(const ShaderProgram* prog, const char* name) {
    if (!prog)
        return NULL;
    for (int i = 0; i < prog->uniform_count; i++) {
        if (strcmp(prog->uniforms[i].name, name) == 0)
            return &prog->uniforms[i];
    }
    return NULL;
}

/* ================================================================== */
/*  Uniform lookup helpers for shaders                                 */
/* ================================================================== */

static Mat4 get_mat4(const ShaderUniform* u, int count, const char* name) {
    for (int i = 0; i < count; i++)
        if (u[i].type == UNIFORM_MAT4 && strcmp(u[i].name, name) == 0)
            return u[i].data.m4;
    return mat4_identity();
}

static Vec3 get_vec3(const ShaderUniform* u, int count, const char* name, Vec3 def) {
    for (int i = 0; i < count; i++)
        if (u[i].type == UNIFORM_VEC3 && strcmp(u[i].name, name) == 0)
            return u[i].data.v3;
    return def;
}

static float get_float(const ShaderUniform* u, int count, const char* name, float def) {
    for (int i = 0; i < count; i++)
        if (u[i].type == UNIFORM_FLOAT && strcmp(u[i].name, name) == 0)
            return u[i].data.f;
    return def;
}

static int get_int(const ShaderUniform* u, int count, const char* name, int def) {
    for (int i = 0; i < count; i++)
        if (u[i].type == UNIFORM_INT && strcmp(u[i].name, name) == 0)
            return u[i].data.i;
    return def;
}

static const XSharpTexture* get_texture(const ShaderUniform* u, int count, const char* name) {
    for (int i = 0; i < count; i++)
        if (u[i].type == UNIFORM_TEXTURE && strcmp(u[i].name, name) == 0)
            return u[i].data.tex;
    return NULL;
}

/* ================================================================== */
/*  Common vertex shader: transforms + writes standard varyings        */
/* ================================================================== */

static VertexOutput common_vertex(const VertexInput* in, const ShaderUniform* u, int uc) {
    VertexOutput out;
    memset(&out, 0, sizeof(out));

    Mat4 mvp = get_mat4(u, uc, "mvp");
    Mat4 model = get_mat4(u, uc, "model");
    Mat4 normal_mat = get_mat4(u, uc, "normal_mat");

    out.position = mat4_mul_vec4(mvp, vec4_from_vec3(in->position, 1.0f));

    Vec3 world_pos = mat4_mul_point(model, in->position);
    Vec3 world_nrm = vec3_normalize(mat4_mul_dir(normal_mat, in->normal));

    /* Varyings: world_pos (0-2), normal (3-5), uv (6-7), color (8-11) */
    out.varyings.v[0] = world_pos.x;
    out.varyings.v[1] = world_pos.y;
    out.varyings.v[2] = world_pos.z;
    out.varyings.v[3] = world_nrm.x;
    out.varyings.v[4] = world_nrm.y;
    out.varyings.v[5] = world_nrm.z;
    out.varyings.v[6] = in->uv.x;
    out.varyings.v[7] = in->uv.y;
    out.varyings.v[8] = in->color.x;
    out.varyings.v[9] = in->color.y;
    out.varyings.v[10] = in->color.z;
    out.varyings.v[11] = in->color.w;

    return out;
}

/* ================================================================== */
/*  Unlit fragment shader                                              */
/* ================================================================== */

static Vec4 unlit_fragment(const FragmentInput* in, const ShaderUniform* u, int uc) {
    Vec4 color = {in->varyings.v[8], in->varyings.v[9], in->varyings.v[10], in->varyings.v[11]};

    const XSharpTexture* tex = get_texture(u, uc, "diffuse_tex");
    if (tex) {
        float uv_u = in->varyings.v[6];
        float uv_v = in->varyings.v[7];
        Color tc = texture_sample(tex, uv_u, uv_v, TEX_FILTER_NEAREST);
        Vec4 tv = color_to_vec4(tc);
        color.x *= tv.x;
        color.y *= tv.y;
        color.z *= tv.z;
        color.w *= tv.w;
    }
    return color;
}

ShaderProgram* shader_create_unlit(void) {
    return shader_create("unlit", common_vertex, unlit_fragment, 12);
}

/* ================================================================== */
/*  Diffuse (Lambert) fragment shader                                  */
/* ================================================================== */

static Vec4 diffuse_fragment(const FragmentInput* in, const ShaderUniform* u, int uc) {
    Vec3 normal = vec3_normalize(vec3(in->varyings.v[3], in->varyings.v[4], in->varyings.v[5]));
    Vec3 light_dir = vec3_normalize(get_vec3(u, uc, "light_dir", vec3(0, -1, 0)));
    Vec3 light_col = get_vec3(u, uc, "light_color", vec3(1, 1, 1));
    Vec3 ambient = get_vec3(u, uc, "ambient", vec3(0.1f, 0.1f, 0.1f));

    float ndl = fmaxf(vec3_dot(normal, vec3_neg(light_dir)), 0.0f);

    Vec4 base_color = {in->varyings.v[8], in->varyings.v[9], in->varyings.v[10],
                       in->varyings.v[11]};

    const XSharpTexture* tex = get_texture(u, uc, "diffuse_tex");
    if (tex) {
        Color tc = texture_sample(tex, in->varyings.v[6], in->varyings.v[7], TEX_FILTER_BILINEAR);
        Vec4 tv = color_to_vec4(tc);
        base_color.x *= tv.x;
        base_color.y *= tv.y;
        base_color.z *= tv.z;
        base_color.w *= tv.w;
    }

    Vec3 diffuse = vec3_add(ambient, vec3_mul(light_col, ndl));
    return vec4(clampf(base_color.x * diffuse.x, 0, 1), clampf(base_color.y * diffuse.y, 0, 1),
                clampf(base_color.z * diffuse.z, 0, 1), base_color.w);
}

ShaderProgram* shader_create_diffuse(void) {
    ShaderProgram* p = shader_create("diffuse", common_vertex, diffuse_fragment, 12);
    if (p) {
        shader_set_vec3(p, "light_dir", vec3(0, -1, 0));
        shader_set_vec3(p, "light_color", vec3(1, 1, 1));
        shader_set_vec3(p, "ambient", vec3(0.1f, 0.1f, 0.1f));
    }
    return p;
}

/* ================================================================== */
/*  Phong fragment shader                                              */
/* ================================================================== */

static Vec4 phong_fragment(const FragmentInput* in, const ShaderUniform* u, int uc) {
    Vec3 world_pos = vec3(in->varyings.v[0], in->varyings.v[1], in->varyings.v[2]);
    Vec3 normal = vec3_normalize(vec3(in->varyings.v[3], in->varyings.v[4], in->varyings.v[5]));
    Vec3 light_dir = vec3_normalize(get_vec3(u, uc, "light_dir", vec3(0, -1, 0)));
    Vec3 light_col = get_vec3(u, uc, "light_color", vec3(1, 1, 1));
    Vec3 ambient = get_vec3(u, uc, "ambient", vec3(0.1f, 0.1f, 0.1f));
    Vec3 eye_pos = get_vec3(u, uc, "eye_pos", vec3(0, 0, 5));
    Vec3 spec_col = get_vec3(u, uc, "specular", vec3(1, 1, 1));
    float shininess = get_float(u, uc, "shininess", 32.0f);

    Vec4 base_color = {in->varyings.v[8], in->varyings.v[9], in->varyings.v[10],
                       in->varyings.v[11]};

    const XSharpTexture* tex = get_texture(u, uc, "diffuse_tex");
    if (tex) {
        Color tc = texture_sample(tex, in->varyings.v[6], in->varyings.v[7], TEX_FILTER_BILINEAR);
        Vec4 tv = color_to_vec4(tc);
        base_color.x *= tv.x;
        base_color.y *= tv.y;
        base_color.z *= tv.z;
        base_color.w *= tv.w;
    }

    /* Diffuse */
    float ndl = fmaxf(vec3_dot(normal, vec3_neg(light_dir)), 0.0f);

    /* Specular (Blinn-Phong) */
    Vec3 view_dir = vec3_normalize(vec3_sub(eye_pos, world_pos));
    Vec3 half_dir = vec3_normalize(vec3_sub(view_dir, light_dir));
    float ndh = fmaxf(vec3_dot(normal, half_dir), 0.0f);
    float spec = powf(ndh, shininess);

    Vec3 color_rgb =
        vec3_add(vec3_add(ambient, vec3_mul(light_col, ndl)), vec3_mul(spec_col, spec));

    return vec4(clampf(base_color.x * color_rgb.x, 0, 1), clampf(base_color.y * color_rgb.y, 0, 1),
                clampf(base_color.z * color_rgb.z, 0, 1), base_color.w);
}

ShaderProgram* shader_create_phong(void) {
    ShaderProgram* p = shader_create("phong", common_vertex, phong_fragment, 12);
    if (p) {
        shader_set_vec3(p, "light_dir", vec3(0, -1, 0));
        shader_set_vec3(p, "light_color", vec3(1, 1, 1));
        shader_set_vec3(p, "ambient", vec3(0.1f, 0.1f, 0.1f));
        shader_set_vec3(p, "eye_pos", vec3(0, 0, 5));
        shader_set_vec3(p, "specular", vec3(1, 1, 1));
        shader_set_float(p, "shininess", 32.0f);
    }
    return p;
}

/* ================================================================== */
/*  Toon / cel-shading fragment shader                                 */
/* ================================================================== */

static Vec4 toon_fragment(const FragmentInput* in, const ShaderUniform* u, int uc) {
    Vec3 normal = vec3_normalize(vec3(in->varyings.v[3], in->varyings.v[4], in->varyings.v[5]));
    Vec3 light_dir = vec3_normalize(get_vec3(u, uc, "light_dir", vec3(0, -1, 0)));
    Vec3 light_col = get_vec3(u, uc, "light_color", vec3(1, 1, 1));
    Vec3 ambient = get_vec3(u, uc, "ambient", vec3(0.1f, 0.1f, 0.1f));
    int levels = get_int(u, uc, "toon_levels", 4);

    Vec4 base_color = {in->varyings.v[8], in->varyings.v[9], in->varyings.v[10],
                       in->varyings.v[11]};

    const XSharpTexture* tex = get_texture(u, uc, "diffuse_tex");
    if (tex) {
        Color tc = texture_sample(tex, in->varyings.v[6], in->varyings.v[7], TEX_FILTER_NEAREST);
        Vec4 tv = color_to_vec4(tc);
        base_color.x *= tv.x;
        base_color.y *= tv.y;
        base_color.z *= tv.z;
        base_color.w *= tv.w;
    }

    float ndl = fmaxf(vec3_dot(normal, vec3_neg(light_dir)), 0.0f);

    /* Quantize the diffuse term */
    if (levels < 1)
        levels = 1;
    float quantized = floorf(ndl * levels + 0.5f) / levels;

    Vec3 lit = vec3_add(ambient, vec3_mul(light_col, quantized));
    return vec4(clampf(base_color.x * lit.x, 0, 1), clampf(base_color.y * lit.y, 0, 1),
                clampf(base_color.z * lit.z, 0, 1), base_color.w);
}

ShaderProgram* shader_create_toon(void) {
    ShaderProgram* p = shader_create("toon", common_vertex, toon_fragment, 12);
    if (p) {
        shader_set_vec3(p, "light_dir", vec3(0, -1, 0));
        shader_set_vec3(p, "light_color", vec3(1, 1, 1));
        shader_set_vec3(p, "ambient", vec3(0.15f, 0.15f, 0.15f));
        shader_set_int(p, "toon_levels", 4);
    }
    return p;
}

/* ================================================================== */
/*  Wireframe shader                                                   */
/* ================================================================== */

static Vec4 wireframe_fragment(const FragmentInput* in, const ShaderUniform* u, int uc) {
    (void)u;
    (void)uc;
    return vec4(in->varyings.v[8], in->varyings.v[9], in->varyings.v[10], in->varyings.v[11]);
}

ShaderProgram* shader_create_wireframe(void) {
    ShaderProgram* p = shader_create("wireframe", common_vertex, wireframe_fragment, 12);
    return p;
}
