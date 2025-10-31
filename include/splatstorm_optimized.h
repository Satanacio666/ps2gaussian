/*
 * SPLATSTORM X - Optimized Implementation Header
 * Complete API for production-ready PS2 Gaussian Splatting engine
 */

#ifndef SPLATSTORM_OPTIMIZED_H
#define SPLATSTORM_OPTIMIZED_H

#include <tamtypes.h>
#include "memory_optimized.h"
#include "gaussian_types.h"

// PackedSplat is defined in memory_optimized.h

/*
 * DMA Optimized Functions
 */
void build_and_send_packet_optimized(PackedSplat* splatArray, int count, float* mvp_matrix);
void process_splats_batched(PackedSplat* splats, int total_count, float* mvp_matrix);
void build_gif_packet_optimized(PackedSplat* splats, int count);
void vu_wait_for_completion(void);
void get_dma_stats(DMAStats* stats);

/*
 * Sorting Optimized Functions
 */
int sorting_system_init(PackedSplat* splats, int count);
void bucket_sort_splats_optimized(void);
void sorting_camera_moved(void);
int camera_moved_significantly(void);
void get_sorting_stats(int* last_sort_frame, int* buckets_used, float* sort_time_ms);
void sorting_system_cleanup(void);

/*
 * GS Optimized Functions
 */
void init_gs_for_splats_optimized(u32 zbuffer_address);
void render_splat_batch_optimized(PackedSplat* splats, int count);
void gs_set_splat_blending_mode(void);
void gs_set_wireframe_mode(int enable);
void gs_configure_depth_test(int enable, int test_method);
void get_gs_stats(u32* pixels_rendered, u32* triangles_rendered, float* fillrate_mpixels);
int gs_optimized_is_initialized(void);
void gs_optimized_cleanup(void);

/*
 * Profiling System Functions
 */
// Types defined in gaussian_types.h - removed to avoid conflicts

int profiling_system_init(void);
void profile_frame_start(void);
void profile_frame_end(void);
void profile_dma_upload_start(void);
void profile_dma_upload_end(void);
void profile_vu_execute_start(void);
void profile_vu_execute_end(void);
void profile_gs_render_start(void);
void profile_gs_render_end(void);
void profile_set_splat_stats(u32 processed, u32 culled);
void profile_set_overdraw_stats(u32 overdraw_pixels);
void profile_get_frame_data(FrameProfileData* data);
void debug_set_visualization_mode(DebugMode mode);
DebugMode debug_get_visualization_mode(void);
void render_debug_overlay(void);
void render_performance_overlay(void);
void profiling_set_enabled(int enabled);
int profiling_is_enabled(void);
void profiling_get_stats_summary(float* avg_fps, float* avg_frame_time, u32* total_frames, u32* avg_splats);
void profiling_reset_stats(void);
void profiling_system_cleanup(void);

/*
 * Utility Functions
 */
u64 get_cpu_cycles(void);
u64 get_cpu_cycles_64(void);
// cycles_to_ms is implemented as static inline in performance_utils.h

/*
 * Performance Constants
 */
#define TARGET_FPS_60           60
#define TARGET_FPS_30           30
#define TARGET_FRAME_TIME_60MS  16.67f
#define TARGET_FRAME_TIME_30MS  33.33f
#define MAX_SPLATS_ULTRA        16000
#define MAX_SPLATS_HIGH         12000
#define MAX_SPLATS_MEDIUM       8000
#define MAX_SPLATS_LOW          4000

/*
 * Quality Levels
 */
typedef enum {
    RENDER_QUALITY_ULTRA,    // 16K splats, full precision
    RENDER_QUALITY_HIGH,     // 12K splats, simplified math
    RENDER_QUALITY_MEDIUM,   // 8K splats, bucket sorting
    RENDER_QUALITY_LOW,      // 4K splats, no alpha
    RENDER_QUALITY_FALLBACK  // Basic triangle rendering
} RenderQuality;

/*
 * Main Rendering Pipeline Functions
 */
int splatstorm_optimized_init(void);
void splatstorm_render_frame_optimized(PackedSplat* splats, int count, float* mvp_matrix);
void splatstorm_set_quality_level(RenderQuality quality);
RenderQuality splatstorm_get_quality_level(void);
void splatstorm_optimized_cleanup(void);

#endif // SPLATSTORM_OPTIMIZED_H