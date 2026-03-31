#ifndef XSHARP_SHADER_H
#define XSHARP_SHADER_H

#include "../core/math3d.h"
#include "../core/renderer.h"
#include "../core/texture.h"

/* ------------------------------------------------------------------ */
/*  Varying interpolation (up to 16 float values per vertex)           */
/* ------------------------------------------------------------------ */

#define SHADER_MAX_VARYINGS 16
#define SHADER_MAX_UNIFORMS 32

typedef struct {
    float v[SHADER_MAX_VARYINGS];
} ShaderVaryings;

/* Interpolate varyings using barycentric coordinates with perspective correction */
static inline ShaderVaryings varying_interpolate(const ShaderVaryings* v0, const ShaderVaryings* v1,
                                                 const ShaderVaryings* v2, float w0, float w1,
                                                 float w2, int count) {
    ShaderVaryings r = {{{0}}};
    for (int i = 0; i < count && i < SHADER_MAX_VARYINGS; i++)
        r.v[i] = v0->v[i] * w0 + v1->v[i] * w1 + v2->v[i] * w2;
    return r;
}

/* ------------------------------------------------------------------ */
/*  Uniform types                                                      */
/* ------------------------------------------------------------------ */

typedef enum {
    UNIFORM_FLOAT,
    UNIFORM_VEC3,
    UNIFORM_VEC4,
    UNIFORM_MAT4,
    UNIFORM_TEXTURE,
    UNIFORM_INT
} UniformType;

typedef struct {
    char name[64];
    UniformType type;
    union {
        float f;
        int i;
        Vec3 v3;
        Vec4 v4;
        Mat4 m4;
        const XSharpTexture* tex;
    } data;
} ShaderUniform;

/* ------------------------------------------------------------------ */
/*  Shader vertex / fragment inputs                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    Vec3 position; /* object-space position */
    Vec3 normal;   /* object-space normal */
    Vec2 uv;
    Vec4 color;
} VertexInput;

typedef struct {
    Vec4 position;           /* clip-space output position */
    ShaderVaryings varyings; /* outputs to fragment shader */
} VertexOutput;

typedef struct {
    ShaderVaryings varyings; /* interpolated from vertex shader */
    Vec2 screen_pos;         /* pixel coordinates */
    float depth;             /* fragment depth */
} FragmentInput;

/* ------------------------------------------------------------------ */
/*  Shader function pointer types                                      */
/* ------------------------------------------------------------------ */

/* Uniforms are passed as an array; vertex/fragment shaders can read them by name index */
typedef VertexOutput (*VertexShaderFn)(const VertexInput* in, const ShaderUniform* uniforms,
                                       int uniform_count);

typedef Vec4 (*FragmentShaderFn)(const FragmentInput* in, const ShaderUniform* uniforms,
                                 int uniform_count);

/* ------------------------------------------------------------------ */
/*  Shader program                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[64];
    VertexShaderFn vertex_fn;
    FragmentShaderFn fragment_fn;
    int varying_count; /* how many varyings are used */
    ShaderUniform uniforms[SHADER_MAX_UNIFORMS];
    int uniform_count;
} ShaderProgram;

ShaderProgram* shader_create(const char* name, VertexShaderFn vs, FragmentShaderFn fs,
                             int varying_count);
void shader_destroy(ShaderProgram* prog);

/* Uniform setters */
void shader_set_float(ShaderProgram* prog, const char* name, float v);
void shader_set_int(ShaderProgram* prog, const char* name, int v);
void shader_set_vec3(ShaderProgram* prog, const char* name, Vec3 v);
void shader_set_vec4(ShaderProgram* prog, const char* name, Vec4 v);
void shader_set_mat4(ShaderProgram* prog, const char* name, Mat4 v);
void shader_set_texture(ShaderProgram* prog, const char* name, const XSharpTexture* tex);

/* Find uniform by name; returns NULL if not found */
const ShaderUniform* shader_get_uniform(const ShaderProgram* prog, const char* name);

/* ------------------------------------------------------------------ */
/*  Built-in shaders                                                   */
/* ------------------------------------------------------------------ */

/*
 * Varying layout for built-in shaders:
 *   0-2: world position (x,y,z)
 *   3-5: world normal (x,y,z)
 *   6-7: UV (u,v)
 *   8-11: vertex color (r,g,b,a)
 *
 * Common uniforms:
 *   "model"        - Mat4
 *   "view"         - Mat4
 *   "projection"   - Mat4
 *   "mvp"          - Mat4 (model * view * projection)
 *   "normal_mat"   - Mat4 (transpose(inverse(model)))
 *   "light_dir"    - Vec3 (directional light direction, normalized)
 *   "light_color"  - Vec3
 *   "ambient"      - Vec3
 *   "eye_pos"      - Vec3
 *   "diffuse_tex"  - Texture
 *   "toon_levels"  - Int (for toon shader)
 *   "shininess"    - Float (for phong)
 *   "specular"     - Vec3
 */

/* Unlit: just vertex color * texture */
ShaderProgram* shader_create_unlit(void);

/* Diffuse (Lambert): N dot L lighting */
ShaderProgram* shader_create_diffuse(void);

/* Phong: diffuse + specular */
ShaderProgram* shader_create_phong(void);

/* Toon / cel shading: quantized N dot L */
ShaderProgram* shader_create_toon(void);

/* Wireframe: emits vertex color (used with wireframe render state) */
ShaderProgram* shader_create_wireframe(void);

#endif /* XSHARP_SHADER_H */
