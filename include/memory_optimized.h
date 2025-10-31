/*
 * SPLATSTORM X - Optimized Memory Management Header
 * 64-byte aligned PackedSplat structure and allocation functions
 */

#ifndef MEMORY_OPTIMIZED_H
#define MEMORY_OPTIMIZED_H

#include <tamtypes.h>
#include <stddef.h>

// Scratchpad memory constants
#define SCRATCHPAD_BASE 0x70000000
#define SCRATCHPAD_SIZE (16 * 1024)  // 16KB

/*
 * PackedSplat Structure - 64-byte aligned for optimal cache performance
 * Layout optimized for VU1 processing and DMA transfers
 */
typedef struct {
    union {
        float pos_color[8];    // pos.xyz + 1.0, color.rgba (32 bytes)
        struct {
            float position[4]; // x, y, z, w
            float color[4];    // r, g, b, a
        };
    };
    union {
        float scale_rot[8];    // scale.xyz + 0.0, rotation.xyzw (32 bytes)
        struct {
            float scale[4];    // x, y, z, w
            float rotation[4]; // x, y, z, w (quaternion)
        };
    };
    u32 color_packed;          // Packed RGBA color for compatibility
    u32 _padding[3];           // Padding to maintain 64-byte alignment
} __attribute__((aligned(64))) PackedSplat;  // Total: 64 bytes

/*
 * DMA Statistics Structure
 */
typedef struct {
    u32 packets_sent;
    u32 bytes_transferred;
    float average_bandwidth;  // MB/s
    u32 vif_stalls;
    u32 vu_idle_time;
} DMAStats;

/*
 * Memory allocation functions
 */
PackedSplat* allocate_splat_array_optimized(int count);
void free_splat_array_optimized(PackedSplat* splats);
void* allocate_vu_buffer(size_t size);
void* allocate_dma_buffer_aligned(size_t size);
void free_dma_buffer_aligned(void* buffer);

/*
 * Scratchpad management
 */
void scratchpad_reset(void);
void* scratchpad_alloc(size_t size);
int scratchpad_available(void);

/*
 * Cache management
 */
void ensure_cache_coherency(void* ptr, size_t size);
void flush_all_caches(void);

#endif // MEMORY_OPTIMIZED_H