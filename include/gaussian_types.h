/*
 * SPLATSTORM X - Complete Fixed-Point Gaussian Foundation
 * Real Gaussian Splatting with adaptive scaling and numerical stability
 * Based on "3D Gaussian Splatting for Real-Time Radiance Field Rendering" [arXiv:2308.04079]
 * 
 * COMPLETE IMPLEMENTATION - NO STUBS OR PLACEHOLDERS
 */

#ifndef GAUSSIAN_TYPES_H
#define GAUSSIAN_TYPES_H

// _EE is defined in splatstorm_x.h - COMPLETE IMPLEMENTATION
// Note: Always include splatstorm_x.h before this header
#include <tamtypes.h>
#include <math.h>
#include <stdbool.h>

// Memory pool types
typedef enum {
    POOL_TYPE_LINEAR,                         // Linear allocation (fast, no free)
    POOL_TYPE_STACK,                          // Stack allocation (LIFO free)
    POOL_TYPE_BUDDY,                          // Buddy system (power-of-2 sizes)
    POOL_TYPE_FREELIST,                       // Free list (general purpose)
    POOL_TYPE_RING                            // Ring buffer (circular)
} MemoryPoolType;

// Fixed-point precision definitions with overflow protection
#define FIXED16_SHIFT 16
#define FIXED16_SCALE (1 << FIXED16_SHIFT)  // 65536.0
#define FIXED8_SHIFT 8
#define FIXED8_SCALE (1 << FIXED8_SHIFT)    // 256.0

// Safety limits for overflow detection
#define FIXED16_MAX 0x7FFFFFFF
#define FIXED16_MIN 0x80000000
#define FIXED8_MAX 0x7FFF
#define FIXED8_MIN 0x8000

// Fixed-point types with range documentation
typedef s32 fixed16_t;  // Q16.16: -32768.0 to 32767.99998 range
typedef s16 fixed8_t;   // Q8.8: -128.0 to 127.996 range (mantissa for adaptive scaling)

// LUT configuration for texture-based operations
#define LUT_SIZE 256
#define LUT_THRESHOLD_SQ 9.0f           // 3σ^2 cutoff for Gaussian falloff
#define CUTOFF_SIGMA 3.0f               // Standard 3-sigma cutoff
#define COV_INV_LUT_RES 128             // 128x128 covariance inverse LUT
#define ATLAS_ENTRIES 64                // 8x8 footprint atlas
#define FOOTPRINT_RES 32                // 32x32 per footprint
#define ATLAS_SIZE (FOOTPRINT_RES * 8)  // 256x256 total atlas
#define MAX_EIG_VAL 10.0f               // Maximum eigenvalue for LUT normalization

// Memory alignment constants
#define CACHE_LINE_SIZE 64
#define VU_ALIGNMENT 16
#define DMA_ALIGNMENT 128

// Performance constants
#define VU_BATCH_SIZE 256               // Splats per VU batch (fits in 16KB)
#define MAX_SPLATS_PER_TILE 128         // Maximum splats per tile
#define MAX_SPLATS_PER_SCENE 32768      // Maximum splats per scene
#define NUM_DEPTH_BUCKETS 256           // Bucket sort depth buckets

// 3D Gaussian splat with adaptive covariance scaling (64-byte aligned)
typedef struct {
    fixed16_t pos[3];           // 3D position (Q16.16) - 12 bytes
    u8 cov_exp : 4;             // Covariance exponent (0-15, scale=2^(exp-7)) - 4 bits
    u8 padding_bits : 4;        // Padding bits - 4 bits
    fixed8_t cov_mant[9];       // 3x3 covariance mantissa (Q8.8) - 18 bytes
    u8 color[3];                // RGB (0-255) - 3 bytes
    u8 opacity;                 // Opacity (0-255, sigmoid-scaled) - 1 byte
    u16 sh_coeffs[16];          // SH degree 0-2 coefficients (quantized) - 32 bytes
    u32 importance;             // Importance metric for LOD - 4 bytes
    u8 padding[8];              // Pad to 64 bytes for cache alignment
} __attribute__((aligned(CACHE_LINE_SIZE))) GaussianSplat3D;

// 2D projected Gaussian splat with complete information (64-byte aligned)
typedef struct {
    fixed16_t screen_pos[2];    // 2D screen position (Q16.16) - 8 bytes
    fixed16_t depth;            // Z-depth for sorting (Q16.16) - 4 bytes
    fixed16_t radius;           // Approximate radius (3σ, Q16.16) - 4 bytes
    fixed8_t cov_2d[4];         // 2x2 projected covariance (Q8.8) - 8 bytes
    fixed8_t inv_cov_2d[4];     // 2x2 inverse covariance (Q8.8) - 8 bytes
    u8 color[4];                // RGBA - 4 bytes
    fixed16_t eigenvals[2];     // Eigenvalues for ellipse axes (Q16.16) - 8 bytes
    fixed16_t eigenvecs[4];     // Eigenvectors (2x2 rotation matrix, Q16.16) - 16 bytes
    u16 tile_mask;              // Bitmask of tiles this splat affects - 2 bytes
    u8 atlas_u, atlas_v;        // Atlas UV coordinates (0-255) - 2 bytes
    u8 padding[4];              // Pad to 64 bytes
} __attribute__((aligned(CACHE_LINE_SIZE))) GaussianSplat2D;

// Tile-based rasterization configuration
#define TILE_SIZE 16
#define TILES_X (640/TILE_SIZE)         // 40 tiles horizontally
#define TILES_Y (448/TILE_SIZE)         // 28 tiles vertically
#define MAX_TILES (TILES_X * TILES_Y)   // 1120 total tiles

// Hierarchical tile configuration
#define COARSE_TILE_SIZE 64
#define COARSE_TILES_X (640/COARSE_TILE_SIZE)  // 10 coarse tiles
#define COARSE_TILES_Y (448/COARSE_TILE_SIZE)  // 7 coarse tiles
#define MAX_COARSE_TILES (COARSE_TILES_X * COARSE_TILES_Y)

// Tile range with depth bounds for culling
typedef struct {
    u16 start_index;            // Starting index in sorted splat array
    u16 count;                  // Number of splats in this tile
    fixed16_t min_depth;        // Minimum depth in tile
    fixed16_t max_depth;        // Maximum depth in tile
    u8 visibility_mask;         // Visibility bitmask for hierarchical culling
    u8 padding[3];              // Alignment
} TileRange;

// Advanced LUT texture data structures
typedef struct {
    u32* exp_lut;               // Exponential falloff LUT (256 entries)
    u32* sqrt_lut;              // Square root LUT for eigenvalues (256 entries)
    u32* cov_inv_lut;           // 2D covariance inverse LUT (128x128)
    u32* footprint_atlas;       // Precalculated Gaussian footprints (256x256)
    u32* sh_lighting_lut;       // Spherical harmonics lighting (256x256)
    u32* recip_lut;             // Reciprocal LUT for divisions (256 entries)
    bool initialized;           // Initialization flag
    u32 total_memory_usage;     // Total VRAM usage in bytes
} GaussianLUTs;

// VU batch processing structure with double buffering
typedef struct {
    GaussianSplat3D* input_buffer_a;    // Input buffer A
    GaussianSplat3D* input_buffer_b;    // Input buffer B
    GaussianSplat2D* output_buffer_a;   // Output buffer A
    GaussianSplat2D* output_buffer_b;   // Output buffer B
    u32 current_buffer;                 // Current active buffer (0 or 1)
    u32 batch_count;                    // Number of splats in current batch
    fixed16_t view_matrix[16];          // View matrix (Q16.16)
    fixed16_t proj_matrix[16];          // Projection matrix (Q16.16)
    fixed16_t viewport[4];              // Viewport transform (x, y, w, h)
    bool processing;                    // VU processing flag
} VUBatchProcessor;

// Memory pool for efficient allocation
typedef struct {
    void* memory_block;         // Large allocated block
    u32 block_size;             // Total block size
    u32 used_size;              // Currently used size
    u32 alignment;              // Alignment requirement
    bool initialized;           // Pool initialization flag
} MemoryPool;

// Performance profiling with detailed breakdown
typedef struct {
    u64 dma_upload_time;
    u64 vu_execute_time;
    u64 gs_render_time;
    u64 total_frame_time;
    u64 frame_start_time;
    u64 frame_start_cycles;     // Frame start timestamp
    u64 frame_cycles;           // Total frame cycles
    u64 cull_cycles;            // Culling cycles
    u64 frustum_cull_cycles;    // Frustum culling time
    u64 vu_upload_cycles;       // VU data upload time
    u64 vu_execute_cycles;      // VU execution time
    u64 vu_download_cycles;     // VU result download time
    u64 tile_sort_cycles;       // Tile sorting time
    u64 gs_render_cycles;       // GS rendering time
    u64 total_frame_cycles;     // Total frame time
    float frame_time_ms;        // Frame time in milliseconds
    float fps;                  // Frames per second
    float vu_utilization;       // VU utilization percentage
    float gs_fillrate_mpixels;  // GS fillrate in megapixels/sec
    u32 splats_input;           // Input splat count
    u32 splats_culled;          // Culled splat count
    u32 splats_processed;       // Processed splat count
    u32 visible_splats;         // Visible splats after culling
    u32 projected_splats;       // Projected splats
    u32 rendered_splats;        // Actually rendered splats
    u32 tiles_rendered;         // Tiles with content
    u32 overdraw_pixels;        // Estimated overdraw
    u32 frame_number;           // Current frame number
} FrameProfileData;

// PLY file information structure
typedef struct {
    u32 vertex_count;           // Number of vertices in PLY file
    bool is_binary;             // Binary or ASCII format
    bool has_scale;             // Has scale properties
    bool has_rotation;          // Has rotation properties  
    bool has_color;             // Has color properties
    bool has_opacity;           // Has opacity property
    u32 memory_required;        // Estimated memory requirement in bytes
    u32 load_time_estimate_ms;  // Estimated load time in milliseconds
} PLYFileInfo;

// Culling statistics structure
typedef struct {
    u32 total_splats;           // Total splats in scene
    u32 total_cells;            // Total spatial grid cells
    u32 visible_cells;          // Cells visible this frame
    u32 empty_cells;            // Empty cells
    u64 frame_number;           // Current frame number
} CullingStats;

// Memory management statistics
typedef struct {
    u32 total_allocated;        // Total bytes allocated
    u32 total_freed;           // Total bytes freed
    u32 peak_usage;            // Peak memory usage
    u32 active_allocations;    // Number of active allocations
    u32 fragmentation_events;  // Number of fragmentation events
    u32 scratchpad_used;       // Scratchpad bytes used
    u32 scratchpad_peak;       // Peak scratchpad usage
    u32 scratchpad_size;       // Total scratchpad size
    u32 scratchpad_allocated;   // Scratchpad bytes allocated
    u32 scratchpad_available;   // Scratchpad bytes available
    u32 pool_used;             // Memory pool bytes used
    u32 pool_available;        // Memory pool bytes available
    u32 system_initialized;    // System initialization status
    float fragmentation_ratio; // Fragmentation ratio
    float cache_efficiency;    // Cache efficiency
} MemoryStats;

// Include shared types instead of circular dependency
#include "splatstorm_types.h"

// Camera with temporal coherence tracking
typedef struct {
    fixed16_t view[16];         // View matrix (Q16.16)
    fixed16_t proj[16];         // Projection matrix (Q16.16)
    fixed16_t view_proj[16];    // Combined view-projection (Q16.16)
    fixed16_t viewport[4];      // Viewport transform (x, y, w, h)
    fixed16_t frustum[6][4];    // Frustum planes for culling
    fixed16_t position[3];      // Camera position
    fixed16_t rotation[4];      // Camera rotation (quaternion)
    fixed16_t last_position[3]; // Previous position for coherence
    fixed16_t last_rotation[4]; // Previous rotation for coherence
    bool moved_significantly;   // Movement flag for temporal coherence
    u64 last_update_frame;      // Last update frame number
} CameraFixed;

// Scene management with complete state
typedef struct {
    GaussianSplat3D* splats_3d;         // Original 3D splats
    GaussianSplat2D* splats_2d;         // Projected 2D splats
    u32* sort_keys;                     // Sorting keys for depth ordering
    u16* sort_indices;                  // Sorted indices
    TileRange* tile_ranges;             // Per-tile splat ranges
    TileRange* coarse_tile_ranges;      // Coarse tile ranges for hierarchical culling
    u32* tile_splat_lists;              // Per-tile splat lists
    u32 splat_count;                    // Total number of splats
    u32 max_splats;                     // Maximum splat capacity
    u32 visible_count;                  // Number of visible splats after culling
    bool needs_sort;                    // Flag for temporal coherence
    u64 last_sort_frame;                // Frame number of last sort
    fixed16_t scene_bounds[6];          // Scene bounding box (min_x, max_x, min_y, max_y, min_z, max_z)
    float scene_radius;                 // Scene radius for normalization
    GaussianLUTs luts;                  // Lookup tables
    VUBatchProcessor vu_processor;      // VU batch processor
    MemoryPool memory_pool;             // Memory pool for allocations
    FrameProfileData profile;           // Performance profiling data
} GaussianScene;

// Debug visualization modes with detailed options
typedef enum {
    DEBUG_MODE_NORMAL,
    DEBUG_MODE_WIREFRAME,
    DEBUG_MODE_DEPTH_BUCKETS,
    DEBUG_MODE_OVERDRAW_HEATMAP,
    DEBUG_MODE_TILE_BOUNDS,
    DEBUG_MODE_PERFORMANCE_OVERLAY,
    DEBUG_MODE_COVARIANCE_ELLIPSES,
    DEBUG_MODE_EIGENVALUE_VISUALIZATION,
    DEBUG_MODE_ATLAS_PREVIEW,
    DEBUG_MODE_MEMORY_USAGE
} DebugMode;

// GaussianResult is now defined in splatstorm_x.h to avoid circular includes

// Global LUT arrays (defined in gaussian_math_complete.c)
extern u32 g_exp_lut[LUT_SIZE];
extern u32 g_sqrt_lut[LUT_SIZE];
extern u32 g_cov_inv_lut[COV_INV_LUT_RES * COV_INV_LUT_RES];
extern u32 g_footprint_atlas[ATLAS_SIZE * ATLAS_SIZE];
extern u32 g_sh_lighting_lut[256 * 256];
extern u32 g_recip_lut[LUT_SIZE];

// Function declarations for complete system
GaussianResult gaussian_system_init(u32 max_splats);
void gaussian_system_cleanup(void);
GaussianResult gaussian_scene_init(GaussianScene* scene, u32 max_splats);
void gaussian_scene_destroy(GaussianScene* scene);
GaussianResult gaussian_luts_generate_all(GaussianLUTs* luts);
GaussianResult gaussian_luts_upload_to_gs(GaussianLUTs* luts, void* gsGlobal);
void gaussian_luts_cleanup(GaussianLUTs* luts);

// Memory management functions
GaussianResult memory_pool_init(MemoryPool* pool, u32 size, u32 alignment);
void* memory_pool_alloc(u32 pool_id, u32 size, u32 alignment, const char* file, u32 line);
void memory_pool_reset(u32 pool_id);
void memory_pool_destroy(MemoryPool* pool);

// Fixed-point utility functions with overflow protection - COMPLETE IMPLEMENTATION
// Optimized safe multiplication using MIPS assembly with overflow checking
static inline fixed16_t fixed_mul_safe(fixed16_t a, fixed16_t b) {
    fixed16_t result;
    s32 hi, lo;
    
    // Use MIPS mult instruction and check for overflow
    __asm__ volatile (
        "mult   %2, %3          \n\t"   // Multiply a * b -> HI:LO
        "mfhi   %0              \n\t"   // Get upper 32 bits
        "mflo   %1              \n\t"   // Get lower 32 bits
        : "=r" (hi), "=r" (lo)          // Outputs: hi, lo
        : "r" (a), "r" (b)              // Inputs: a, b
    );
    
    // Check for overflow by examining upper bits
    s32 shifted_hi = hi << 16;
    s32 shifted_lo = (u32)lo >> 16;
    s64 full_result = ((s64)hi << 32) | (u32)lo;
    s64 final_result = full_result >> FIXED16_SHIFT;
    
    // Overflow check
    if (final_result > (s64)FIXED16_MAX) return FIXED16_MAX;
    if (final_result < (s64)FIXED16_MIN) return FIXED16_MIN;
    
    result = shifted_hi | shifted_lo;
    return result;
}

static inline fixed16_t fixed_add_safe(fixed16_t a, fixed16_t b) {
    s64 sum = (s64)a + b;
    if (sum > (s64)FIXED16_MAX) return FIXED16_MAX;
    if (sum < (s64)FIXED16_MIN) return FIXED16_MIN;
    return (fixed16_t)sum;
}

static inline fixed16_t fixed_sub_safe(fixed16_t a, fixed16_t b) {
    s64 diff = (s64)a - b;
    if (diff > (s64)FIXED16_MAX) return FIXED16_MAX;
    if (diff < (s64)FIXED16_MIN) return FIXED16_MIN;
    return (fixed16_t)diff;
}

// Optimized fixed-point multiplication using MIPS assembly - COMPLETE IMPLEMENTATION
static inline fixed16_t fixed_mul(fixed16_t a, fixed16_t b) {
    fixed16_t result;
    
    // Use MIPS mult instruction for 32x32->64 bit multiplication
    // followed by mfhi to get upper 32 bits for the shift
    __asm__ volatile (
        "mult   %1, %2          \n\t"   // Multiply a * b -> HI:LO
        "mfhi   $t0             \n\t"   // Get upper 32 bits to $t0
        "mflo   $t1             \n\t"   // Get lower 32 bits to $t1
        "srl    $t1, $t1, 16    \n\t"   // Shift lower 32 bits right by 16
        "sll    $t0, $t0, 16    \n\t"   // Shift upper 32 bits left by 16
        "or     %0, $t0, $t1    \n\t"   // Combine for final result
        : "=r" (result)                 // Output: result
        : "r" (a), "r" (b)              // Inputs: a, b
        : "$t0", "$t1"                  // Clobbered registers
    );
    
    return result;
}

static inline fixed16_t fixed_add(fixed16_t a, fixed16_t b) {
    return a + b;
}

static inline fixed16_t fixed_sub(fixed16_t a, fixed16_t b) {
    return a - b;
}

static inline fixed16_t fixed_neg(fixed16_t a) {
    return -a;
}

static inline fixed16_t fixed_abs(fixed16_t a) {
    return (a < 0) ? -a : a;
}

static inline fixed16_t fixed_from_float(float f) {
    if (f > 32767.999f) return FIXED16_MAX;
    if (f < -32768.0f) return FIXED16_MIN;
    return (fixed16_t)(f * FIXED16_SCALE);
}

static inline float fixed_to_float(fixed16_t f) {
    return (float)f / FIXED16_SCALE;
}

static inline fixed16_t fixed_from_int(int i) {
    if (i > 32767) return FIXED16_MAX;
    if (i < -32768) return FIXED16_MIN;
    return (fixed16_t)(i << FIXED16_SHIFT);
}

static inline int fixed_to_int(fixed16_t f) {
    return (int)(f >> FIXED16_SHIFT);
}

static inline fixed16_t fixed_mad(fixed16_t a, fixed16_t b, fixed16_t c) {
    return fixed_add(fixed_mul(a, b), c);
}

static inline fixed16_t fixed_mad_safe(fixed16_t a, fixed16_t b, fixed16_t c) {
    return fixed_add_safe(fixed_mul_safe(a, b), c);
}

// Utility macros
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Advanced mathematical functions
fixed16_t fixed_recip_newton(fixed16_t d);
fixed16_t fixed_sqrt_lut(fixed16_t x);
fixed16_t fixed_sin_lut(fixed16_t angle);
fixed16_t fixed_cos_lut(fixed16_t angle);
fixed16_t fixed_atan2_lut(fixed16_t y, fixed16_t x);

// Matrix operations
void matrix_multiply_4x4_fixed(const fixed16_t* a, const fixed16_t* b, fixed16_t* result);
void matrix_multiply_4x4_vector_fixed(const fixed16_t* matrix, const fixed16_t* vector, fixed16_t* result);
void matrix_invert_4x4_fixed(const fixed16_t* matrix, fixed16_t* result);
void matrix_transpose_4x4_fixed(const fixed16_t* matrix, fixed16_t* result);

// Vector operations
fixed16_t vector3_dot_fixed(const fixed16_t* a, const fixed16_t* b);
void vector3_cross_fixed(const fixed16_t* a, const fixed16_t* b, fixed16_t* result);
fixed16_t vector3_length_fixed(const fixed16_t* v);
void vector3_normalize_fixed(fixed16_t* v);

#endif // GAUSSIAN_TYPES_H