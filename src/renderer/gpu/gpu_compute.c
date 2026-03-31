#include "gpu_compute.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ================================================================== */
/*  OpenCL backend (conditionally compiled)                            */
/* ================================================================== */

#ifdef XSHARP_GPU_OPENCL
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

/* ================================================================== */
/*  Internal structures                                                */
/* ================================================================== */

#define GPU_MAX_KERNEL_ARGS 16

struct GpuBuffer_s {
    size_t      size;
    GpuMemFlags flags;
    void       *cpu_data;       /* CPU fallback storage */
#ifdef XSHARP_GPU_OPENCL
    cl_mem      cl_buffer;
#endif
};

struct GpuKernel_s {
    char       name[128];
    CpuKernelFn cpu_fn;
    /* CPU kernel argument pointers */
    void       *cpu_args[GPU_MAX_KERNEL_ARGS];
    size_t      cpu_arg_sizes[GPU_MAX_KERNEL_ARGS];
    bool        cpu_arg_is_value[GPU_MAX_KERNEL_ARGS];
    int         arg_count;
#ifdef XSHARP_GPU_OPENCL
    cl_kernel   cl_kernel;
#endif
};

struct GpuContext_s {
    GpuBackend  backend;
    char        device_name[256];
#ifdef XSHARP_GPU_OPENCL
    cl_context       cl_ctx;
    cl_command_queue  cl_queue;
    cl_device_id      cl_device;
    cl_platform_id    cl_platform;
#endif
};

/* ================================================================== */
/*  Context creation                                                   */
/* ================================================================== */

GpuContext *gpu_context_create(void) {
    GpuContext *ctx = calloc(1, sizeof(GpuContext));
    if (!ctx) return NULL;

#ifdef XSHARP_GPU_OPENCL
    /* Try to initialize OpenCL */
    cl_int err;
    cl_uint num_platforms;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err == CL_SUCCESS && num_platforms > 0) {
        err = clGetPlatformIDs(1, &ctx->cl_platform, NULL);
        if (err == CL_SUCCESS) {
            /* Try GPU first, fall back to CPU device */
            err = clGetDeviceIDs(ctx->cl_platform, CL_DEVICE_TYPE_GPU, 1, &ctx->cl_device, NULL);
            if (err != CL_SUCCESS)
                err = clGetDeviceIDs(ctx->cl_platform, CL_DEVICE_TYPE_CPU, 1, &ctx->cl_device, NULL);

            if (err == CL_SUCCESS) {
                ctx->cl_ctx = clCreateContext(NULL, 1, &ctx->cl_device, NULL, NULL, &err);
                if (err == CL_SUCCESS) {
                    ctx->cl_queue = clCreateCommandQueue(ctx->cl_ctx, ctx->cl_device, 0, &err);
                    if (err == CL_SUCCESS) {
                        ctx->backend = GPU_BACKEND_OPENCL;
                        clGetDeviceInfo(ctx->cl_device, CL_DEVICE_NAME,
                                        sizeof(ctx->device_name), ctx->device_name, NULL);
                        return ctx;
                    }
                    clReleaseContext(ctx->cl_ctx);
                }
            }
        }
    }
    /* Fall through to CPU backend if OpenCL init failed */
#endif

    ctx->backend = GPU_BACKEND_CPU;
    strncpy(ctx->device_name, "CPU (software fallback)", sizeof(ctx->device_name) - 1);
    return ctx;
}

void gpu_context_destroy(GpuContext *ctx) {
    if (!ctx) return;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        if (ctx->cl_queue) clReleaseCommandQueue(ctx->cl_queue);
        if (ctx->cl_ctx) clReleaseContext(ctx->cl_ctx);
    }
#endif

    free(ctx);
}

GpuBackend gpu_get_backend(const GpuContext *ctx) {
    return ctx ? ctx->backend : GPU_BACKEND_NONE;
}

const char *gpu_get_backend_name(const GpuContext *ctx) {
    if (!ctx) return "none";
    switch (ctx->backend) {
    case GPU_BACKEND_NONE:   return "none";
    case GPU_BACKEND_CPU:    return "cpu";
    case GPU_BACKEND_OPENCL: return "opencl";
    }
    return "unknown";
}

const char *gpu_get_device_name(const GpuContext *ctx) {
    return ctx ? ctx->device_name : "unknown";
}

/* ================================================================== */
/*  Buffer management                                                  */
/* ================================================================== */

GpuBuffer *gpu_buffer_create(GpuContext *ctx, size_t size, GpuMemFlags flags) {
    if (!ctx || size == 0) return NULL;

    GpuBuffer *buf = calloc(1, sizeof(GpuBuffer));
    if (!buf) return NULL;
    buf->size  = size;
    buf->flags = flags;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_mem_flags cl_flags = 0;
        switch (flags) {
        case GPU_MEM_READ_ONLY:  cl_flags = CL_MEM_READ_ONLY;  break;
        case GPU_MEM_WRITE_ONLY: cl_flags = CL_MEM_WRITE_ONLY; break;
        case GPU_MEM_READ_WRITE: cl_flags = CL_MEM_READ_WRITE; break;
        }
        cl_int err;
        buf->cl_buffer = clCreateBuffer(ctx->cl_ctx, cl_flags, size, NULL, &err);
        if (err != CL_SUCCESS) { free(buf); return NULL; }
        return buf;
    }
#endif

    /* CPU fallback */
    buf->cpu_data = calloc(1, size);
    if (!buf->cpu_data) { free(buf); return NULL; }
    return buf;
}

void gpu_buffer_destroy(GpuContext *ctx, GpuBuffer *buf) {
    if (!buf) return;

#ifdef XSHARP_GPU_OPENCL
    if (ctx && ctx->backend == GPU_BACKEND_OPENCL && buf->cl_buffer)
        clReleaseMemObject(buf->cl_buffer);
#endif
    (void)ctx;

    free(buf->cpu_data);
    free(buf);
}

GpuError gpu_buffer_upload(GpuContext *ctx, GpuBuffer *buf,
                            const void *data, size_t offset, size_t size) {
    if (!ctx || !buf || !data) return GPU_ERROR_INVALID_ARG;
    if (offset + size > buf->size) return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_int err = clEnqueueWriteBuffer(ctx->cl_queue, buf->cl_buffer, CL_TRUE,
                                           offset, size, data, 0, NULL, NULL);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_ALLOC;
    }
#endif

    memcpy((uint8_t *)buf->cpu_data + offset, data, size);
    return GPU_OK;
}

GpuError gpu_buffer_download(GpuContext *ctx, GpuBuffer *buf,
                              void *data, size_t offset, size_t size) {
    if (!ctx || !buf || !data) return GPU_ERROR_INVALID_ARG;
    if (offset + size > buf->size) return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_int err = clEnqueueReadBuffer(ctx->cl_queue, buf->cl_buffer, CL_TRUE,
                                          offset, size, data, 0, NULL, NULL);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_ALLOC;
    }
#endif

    memcpy(data, (uint8_t *)buf->cpu_data + offset, size);
    return GPU_OK;
}

/* ================================================================== */
/*  Kernel management                                                  */
/* ================================================================== */

GpuKernel *gpu_kernel_create(GpuContext *ctx,
                              const char *source,
                              const char *kernel_name,
                              CpuKernelFn cpu_fn) {
    if (!ctx) return NULL;

    GpuKernel *k = calloc(1, sizeof(GpuKernel));
    if (!k) return NULL;
    if (kernel_name) strncpy(k->name, kernel_name, sizeof(k->name) - 1);
    k->cpu_fn    = cpu_fn;
    k->arg_count = 0;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL && source && kernel_name) {
        cl_int err;
        size_t src_len = strlen(source);
        cl_program prog = clCreateProgramWithSource(ctx->cl_ctx, 1, &source, &src_len, &err);
        if (err != CL_SUCCESS) { free(k); return NULL; }

        err = clBuildProgram(prog, 1, &ctx->cl_device, "-cl-fast-relaxed-math", NULL, NULL);
        if (err != CL_SUCCESS) {
            /* Get build log for debugging */
            size_t log_size;
            clGetProgramBuildInfo(prog, ctx->cl_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
            if (log_size > 1) {
                char *log = malloc(log_size);
                if (log) {
                    clGetProgramBuildInfo(prog, ctx->cl_device, CL_PROGRAM_BUILD_LOG,
                                          log_size, log, NULL);
                    fprintf(stderr, "OpenCL build error:\n%s\n", log);
                    free(log);
                }
            }
            clReleaseProgram(prog);
            free(k);
            return NULL;
        }

        k->cl_kernel = clCreateKernel(prog, kernel_name, &err);
        clReleaseProgram(prog);
        if (err != CL_SUCCESS) { free(k); return NULL; }
        return k;
    }
#else
    (void)source;
    (void)kernel_name;
#endif

    /* CPU backend: need a cpu_fn */
    if (!cpu_fn) { free(k); return NULL; }
    return k;
}

void gpu_kernel_destroy(GpuContext *ctx, GpuKernel *kernel) {
    if (!kernel) return;

#ifdef XSHARP_GPU_OPENCL
    if (ctx && ctx->backend == GPU_BACKEND_OPENCL && kernel->cl_kernel)
        clReleaseKernel(kernel->cl_kernel);
#endif
    (void)ctx;

    /* Free value-type argument copies */
    for (int i = 0; i < kernel->arg_count; i++) {
        if (kernel->cpu_arg_is_value[i])
            free(kernel->cpu_args[i]);
    }
    free(kernel);
}

GpuError gpu_kernel_set_buffer(GpuContext *ctx, GpuKernel *kernel,
                                int arg_index, GpuBuffer *buf) {
    if (!ctx || !kernel || !buf || arg_index < 0 || arg_index >= GPU_MAX_KERNEL_ARGS)
        return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_int err = clSetKernelArg(kernel->cl_kernel, arg_index,
                                     sizeof(cl_mem), &buf->cl_buffer);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_KERNEL;
    }
#endif

    /* CPU: store pointer to cpu_data */
    if (kernel->cpu_arg_is_value[arg_index])
        free(kernel->cpu_args[arg_index]);
    kernel->cpu_args[arg_index] = buf->cpu_data;
    kernel->cpu_arg_sizes[arg_index] = buf->size;
    kernel->cpu_arg_is_value[arg_index] = false;
    if (arg_index >= kernel->arg_count)
        kernel->arg_count = arg_index + 1;
    return GPU_OK;
}

GpuError gpu_kernel_set_value(GpuContext *ctx, GpuKernel *kernel,
                               int arg_index, const void *value, size_t size) {
    if (!ctx || !kernel || !value || arg_index < 0 || arg_index >= GPU_MAX_KERNEL_ARGS)
        return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_int err = clSetKernelArg(kernel->cl_kernel, arg_index, size, value);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_KERNEL;
    }
#endif

    /* CPU: make a copy of the value */
    if (kernel->cpu_arg_is_value[arg_index])
        free(kernel->cpu_args[arg_index]);
    void *copy = malloc(size);
    if (!copy) return GPU_ERROR_ALLOC;
    memcpy(copy, value, size);
    kernel->cpu_args[arg_index] = copy;
    kernel->cpu_arg_sizes[arg_index] = size;
    kernel->cpu_arg_is_value[arg_index] = true;
    if (arg_index >= kernel->arg_count)
        kernel->arg_count = arg_index + 1;
    return GPU_OK;
}

/* ================================================================== */
/*  Dispatch                                                           */
/* ================================================================== */

GpuError gpu_dispatch(GpuContext *ctx, GpuKernel *kernel,
                       uint32_t global_size, uint32_t local_size) {
    if (!ctx || !kernel) return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        size_t gs = global_size;
        size_t ls = local_size > 0 ? local_size : 0;
        cl_int err = clEnqueueNDRangeKernel(ctx->cl_queue, kernel->cl_kernel, 1,
                                             NULL, &gs, ls > 0 ? &ls : NULL,
                                             0, NULL, NULL);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_KERNEL;
    }
#endif

    /* CPU fallback: serial execution */
    if (!kernel->cpu_fn) return GPU_ERROR_KERNEL;
    (void)local_size;
    for (uint32_t i = 0; i < global_size; i++)
        kernel->cpu_fn(i, kernel->cpu_args, kernel->arg_count);
    return GPU_OK;
}

GpuError gpu_dispatch_2d(GpuContext *ctx, GpuKernel *kernel,
                          uint32_t gx, uint32_t gy,
                          uint32_t lx, uint32_t ly) {
    if (!ctx || !kernel) return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        size_t gs[2] = {gx, gy};
        size_t ls[2] = {lx, ly};
        bool use_local = (lx > 0 && ly > 0);
        cl_int err = clEnqueueNDRangeKernel(ctx->cl_queue, kernel->cl_kernel, 2,
                                             NULL, gs, use_local ? ls : NULL,
                                             0, NULL, NULL);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_KERNEL;
    }
#endif

    /* CPU fallback: linearize 2D dispatch */
    if (!kernel->cpu_fn) return GPU_ERROR_KERNEL;
    (void)lx; (void)ly;
    for (uint32_t y = 0; y < gy; y++)
        for (uint32_t x = 0; x < gx; x++)
            kernel->cpu_fn(y * gx + x, kernel->cpu_args, kernel->arg_count);
    return GPU_OK;
}

GpuError gpu_sync(GpuContext *ctx) {
    if (!ctx) return GPU_ERROR_INVALID_ARG;

#ifdef XSHARP_GPU_OPENCL
    if (ctx->backend == GPU_BACKEND_OPENCL) {
        cl_int err = clFinish(ctx->cl_queue);
        return (err == CL_SUCCESS) ? GPU_OK : GPU_ERROR_SYNC;
    }
#endif

    /* CPU is synchronous; nothing to wait for */
    return GPU_OK;
}
