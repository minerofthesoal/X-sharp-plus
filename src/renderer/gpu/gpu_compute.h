#ifndef XSHARP_GPU_COMPUTE_H
#define XSHARP_GPU_COMPUTE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Compile-time backend selection                                     */
/*                                                                     */
/*  Define XSHARP_GPU_OPENCL to enable OpenCL backend.                 */
/*  Otherwise, falls back to CPU compute path.                         */
/* ------------------------------------------------------------------ */

typedef enum {
    GPU_BACKEND_NONE,
    GPU_BACKEND_CPU,
    GPU_BACKEND_OPENCL
} GpuBackend;

typedef enum {
    GPU_MEM_READ_ONLY,
    GPU_MEM_WRITE_ONLY,
    GPU_MEM_READ_WRITE
} GpuMemFlags;

typedef enum {
    GPU_OK = 0,
    GPU_ERROR_INIT,
    GPU_ERROR_ALLOC,
    GPU_ERROR_KERNEL,
    GPU_ERROR_SYNC,
    GPU_ERROR_INVALID_ARG,
    GPU_ERROR_NOT_SUPPORTED
} GpuError;

/* Opaque handle types */
typedef struct GpuBuffer_s  GpuBuffer;
typedef struct GpuKernel_s  GpuKernel;

/* ------------------------------------------------------------------ */
/*  CPU kernel function type                                           */
/*  Called once per work item: fn(global_id, args, arg_count)          */
/* ------------------------------------------------------------------ */

typedef void (*CpuKernelFn)(uint32_t global_id, void **args, int arg_count);

/* ------------------------------------------------------------------ */
/*  GPU compute context                                                */
/* ------------------------------------------------------------------ */

typedef struct GpuContext_s GpuContext;

/* Initialize the compute subsystem.
 * Tries OpenCL if compiled with XSHARP_GPU_OPENCL, else uses CPU. */
GpuContext *gpu_context_create(void);
void        gpu_context_destroy(GpuContext *ctx);

/* Query active backend */
GpuBackend  gpu_get_backend(const GpuContext *ctx);
const char *gpu_get_backend_name(const GpuContext *ctx);
const char *gpu_get_device_name(const GpuContext *ctx);

/* ------------------------------------------------------------------ */
/*  Buffer management                                                  */
/* ------------------------------------------------------------------ */

GpuBuffer *gpu_buffer_create(GpuContext *ctx, size_t size, GpuMemFlags flags);
void       gpu_buffer_destroy(GpuContext *ctx, GpuBuffer *buf);

/* Upload host data to GPU buffer */
GpuError   gpu_buffer_upload(GpuContext *ctx, GpuBuffer *buf,
                              const void *data, size_t offset, size_t size);

/* Download GPU buffer to host memory */
GpuError   gpu_buffer_download(GpuContext *ctx, GpuBuffer *buf,
                                void *data, size_t offset, size_t size);

/* ------------------------------------------------------------------ */
/*  Kernel management                                                  */
/* ------------------------------------------------------------------ */

/* Create a kernel from OpenCL source (if available) or a CPU function pointer.
 * For CPU backend, pass NULL for source and provide cpu_fn.
 * For OpenCL backend, pass source code and kernel_name. */
GpuKernel *gpu_kernel_create(GpuContext *ctx,
                              const char *source,
                              const char *kernel_name,
                              CpuKernelFn cpu_fn);
void       gpu_kernel_destroy(GpuContext *ctx, GpuKernel *kernel);

/* Set kernel arguments */
GpuError   gpu_kernel_set_buffer(GpuContext *ctx, GpuKernel *kernel,
                                  int arg_index, GpuBuffer *buf);
GpuError   gpu_kernel_set_value(GpuContext *ctx, GpuKernel *kernel,
                                 int arg_index, const void *value, size_t size);

/* ------------------------------------------------------------------ */
/*  Dispatch                                                           */
/* ------------------------------------------------------------------ */

/* Dispatch a 1D compute workload */
GpuError   gpu_dispatch(GpuContext *ctx, GpuKernel *kernel,
                         uint32_t global_size, uint32_t local_size);

/* Dispatch a 2D compute workload */
GpuError   gpu_dispatch_2d(GpuContext *ctx, GpuKernel *kernel,
                            uint32_t gx, uint32_t gy,
                            uint32_t lx, uint32_t ly);

/* Wait for all commands to complete */
GpuError   gpu_sync(GpuContext *ctx);

#endif /* XSHARP_GPU_COMPUTE_H */
