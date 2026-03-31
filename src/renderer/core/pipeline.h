#ifndef XSHARP_PIPELINE_H
#define XSHARP_PIPELINE_H

#include "../shaders/shader.h"
#include "mesh.h"
#include "renderer.h"
#include "texture.h"

/* ------------------------------------------------------------------ */
/*  Cull mode                                                          */
/* ------------------------------------------------------------------ */

typedef enum { CULL_NONE, CULL_BACK, CULL_FRONT } CullMode;

/* ------------------------------------------------------------------ */
/*  Fog                                                                */
/* ------------------------------------------------------------------ */

typedef enum { FOG_NONE, FOG_LINEAR, FOG_EXPONENTIAL, FOG_EXPONENTIAL2 } FogMode;

typedef struct {
    FogMode mode;
    Vec4 color;    /* fog color RGBA */
    float start;   /* linear fog start distance */
    float end;     /* linear fog end distance */
    float density; /* exp/exp2 density */
} FogParams;

/* ------------------------------------------------------------------ */
/*  Pipeline state                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    ShaderProgram* shader;
    const XSharpTexture* diffuse_texture;
    bool depth_test;
    bool depth_write;
    BlendMode blend_mode;
    CullMode cull_mode;
    bool wireframe;
    FogParams fog;

    /* Shadow mapping */
    const Framebuffer* shadow_map;
    Mat4 light_vp;
    float shadow_bias;
    bool shadow_enabled;
} PipelineState;

/* Initialize pipeline state with reasonable defaults */
PipelineState pipeline_state_default(void);

/* ------------------------------------------------------------------ */
/*  Pipeline configuration (kept for backward compat)                  */
/* ------------------------------------------------------------------ */

typedef struct {
    ShaderProgram* shader;
    const XSharpTexture* diffuse_texture;
    /* Shadow mapping */
    const Framebuffer* shadow_map;
    Mat4 light_vp;
    float shadow_bias;
    bool shadow_enabled;
} PipelineMaterial;

typedef struct {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 mvp;        /* precomputed model * view * projection */
    Mat4 normal_mat; /* transpose(inverse(model)) */
    Vec3 eye_pos;
} PipelineTransforms;

/* ------------------------------------------------------------------ */
/*  Pipeline object                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    PipelineState state;
    PipelineTransforms xforms;
    XSharpRenderer* renderer;
} Pipeline;

/* Create / destroy pipeline */
Pipeline* pipeline_create(XSharpRenderer* renderer);
void pipeline_destroy(Pipeline* pipe);

/* Bind a pipeline state (shader, depth, blend, cull, wireframe, fog) */
void pipeline_bind(Pipeline* pipe, const PipelineState* state);

/* Set transforms on the pipeline */
void pipeline_set_transforms(Pipeline* pipe, Mat4 model, Mat4 view, Mat4 projection);

/* Draw a mesh through the full pipeline */
void pipeline_draw_mesh(Pipeline* pipe, const Mesh* mesh);

/* Draw a single 3D triangle through the full vertex->clip->raster->fragment chain */
void pipeline_draw_triangle_3d(Pipeline* pipe, const Vertex* v0, const Vertex* v1,
                               const Vertex* v2);

/* ------------------------------------------------------------------ */
/*  Legacy / convenience functions                                     */
/* ------------------------------------------------------------------ */

/* Compute derived transform matrices */
PipelineTransforms pipeline_compute_transforms(Mat4 model, Mat4 view, Mat4 projection);

/*
 * Full pipeline: vertex transform -> clip -> rasterize -> shade -> output
 * Renders a mesh with given transforms and material through the pipeline.
 */
void pipeline_render_mesh(XSharpRenderer* renderer, const Mesh* mesh,
                          const PipelineTransforms* xforms, const PipelineMaterial* material);

/*
 * Render a single triangle through the pipeline (vertex processing,
 * clipping, perspective divide, viewport transform, rasterization).
 */
void pipeline_render_triangle(XSharpRenderer* renderer, const Vertex* v0, const Vertex* v1,
                              const Vertex* v2, const PipelineTransforms* xforms,
                              const PipelineMaterial* material);

#endif /* XSHARP_PIPELINE_H */
