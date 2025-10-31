/*
 * SPLATSTORM X - Comprehensive Profiling System
 * Cycle-accurate performance measurement and debug visualization
 */

#include <tamtypes.h>
#include <kernel.h>
#include <string.h>
#include "splatstorm_debug.h"
#include "splatstorm_x.h"
#include "memory_optimized.h"
#include "performance_utils.h"
#include "graphics_utils.h"

// Forward declaration only for internal function
void render_performance_overlay(void);

// FrameProfileData and DebugMode are defined in gaussian_types.h

static FrameProfileData frame_profile = {0};
static DebugMode current_debug_mode = DEBUG_MODE_NORMAL;
static int profiling_enabled = 1;

// get_cpu_cycles_64 and cycles_to_ms are declared in splatstorm_optimized.h

/*
 * Initialize profiling system
 */
int profiling_system_init(void) {
    memset(&frame_profile, 0, sizeof(frame_profile));
    profiling_enabled = 1;
    current_debug_mode = DEBUG_MODE_NORMAL;
    
    debug_log_info("Profiling system initialized");
    return 0;
}

/*
 * Start frame profiling
 */
void profile_frame_start(void) {
    if (!profiling_enabled) return;
    
    frame_profile.frame_start_time = get_cpu_cycles_64();
    frame_profile.frame_start_cycles = frame_profile.frame_start_time;
    frame_profile.frame_number++;
}

/*
 * Start DMA upload profiling
 */
void profile_dma_upload_start(void) {
    if (!profiling_enabled) return;
    
    frame_profile.dma_upload_time = get_cpu_cycles_64();
}

/*
 * End DMA upload profiling
 */
void profile_dma_upload_end(void) {
    if (!profiling_enabled) return;
    
    frame_profile.dma_upload_time = get_cpu_cycles_64() - frame_profile.dma_upload_time;
}

/*
 * Start VU execution profiling
 */
void profile_vu_execute_start(void) {
    if (!profiling_enabled) return;
    
    frame_profile.vu_execute_time = get_cpu_cycles_64();
}

/*
 * End VU execution profiling
 */
void profile_vu_execute_end(void) {
    if (!profiling_enabled) return;
    
    frame_profile.vu_execute_time = get_cpu_cycles_64() - frame_profile.vu_execute_time;
}

/*
 * Start GS rendering profiling
 */
void profile_gs_render_start(void) {
    if (!profiling_enabled) return;
    
    frame_profile.gs_render_time = get_cpu_cycles_64();
}

/*
 * End GS rendering profiling
 */
void profile_gs_render_end(void) {
    if (!profiling_enabled) return;
    
    frame_profile.gs_render_time = get_cpu_cycles_64() - frame_profile.gs_render_time;
}

/*
 * End frame profiling and output statistics
 */
void profile_frame_end(void) {
    if (!profiling_enabled) return;
    
    frame_profile.total_frame_time = get_cpu_cycles_64() - frame_profile.total_frame_time;
    
    // Convert cycles to milliseconds
    float ms_total = cycles_to_ms(frame_profile.total_frame_time);
    float ms_dma = cycles_to_ms(frame_profile.dma_upload_time);
    float ms_vu = cycles_to_ms(frame_profile.vu_execute_time);
    float ms_gs = cycles_to_ms(frame_profile.gs_render_time);
    
    // Calculate frame rate
    float fps = (ms_total > 0) ? (1000.0f / ms_total) : 0.0f;
    
    // Log performance data (can be disabled in release builds)
    if (frame_profile.frame_number % 60 == 0) {  // Log every 60 frames
        debug_log_info("Frame %d: %.2fms (%.1f FPS) - DMA: %.2fms, VU: %.2fms, GS: %.2fms - %d splats",
                       frame_profile.frame_number, ms_total, fps, ms_dma, ms_vu, ms_gs, 
                       frame_profile.splats_processed);
    }
    
    // Check for performance issues
    if (ms_total > 20.0f) {  // Slower than 50 FPS
        debug_log_warning("Performance warning: Frame time %.2fms (%.1f FPS)", ms_total, fps);
    }
}

/*
 * Set splat processing statistics
 */
void profile_set_splat_stats(u32 processed, u32 culled) {
    if (!profiling_enabled) return;
    
    frame_profile.splats_processed = processed;
    frame_profile.splats_culled = culled;
}

/*
 * Set overdraw statistics
 */
void profile_set_overdraw_stats(u32 overdraw_pixels) {
    if (!profiling_enabled) return;
    
    frame_profile.overdraw_pixels = overdraw_pixels;
}

/*
 * Get current frame profile data
 */
void profile_get_frame_data(FrameProfileData* data) {
    if (data) {
        *data = frame_profile;
    }
}

/*
 * Set debug visualization mode
 */
void debug_set_visualization_mode(DebugMode mode) {
    current_debug_mode = mode;
    
    const char* mode_names[] = {
        "Normal", "Wireframe", "Depth Buckets", "Overdraw Heatmap", "Performance Overlay"
    };
    
    debug_log_info("Debug visualization mode: %s", mode_names[mode]);
}

/*
 * Get current debug visualization mode
 */
DebugMode debug_get_visualization_mode(void) {
    return current_debug_mode;
}

/*
 * Render debug overlay based on current mode
 */
void render_debug_overlay(void) {
    if (!profiling_enabled) return;
    
    switch (current_debug_mode) {
        case DEBUG_MODE_NORMAL:
            // No overlay
            break;
            
        case DEBUG_MODE_WIREFRAME:
            // Enable wireframe rendering in GS
            gs_set_wireframe_mode(1);
            debug_log_info("Wireframe mode active");
            break;
            
        case DEBUG_MODE_DEPTH_BUCKETS:
            // Color-code splats by depth bucket
            debug_log_info("Depth bucket visualization active");
            // Implementation would modify splat colors based on depth
            break;
            
        case DEBUG_MODE_OVERDRAW_HEATMAP:
            // Visualize overdraw by counting fragments per pixel
            debug_log_info("Overdraw heatmap active (overdraw: %d pixels)", 
                          frame_profile.overdraw_pixels);
            break;
            
        case DEBUG_MODE_PERFORMANCE_OVERLAY:
            // Render on-screen performance statistics
            render_performance_overlay();
            break;
            
        case DEBUG_MODE_TILE_BOUNDS:
            // Visualize tile boundaries
            debug_log_info("Tile bounds visualization active");
            break;
            
        case DEBUG_MODE_COVARIANCE_ELLIPSES:
            // Show covariance ellipses for splats
            debug_log_info("Covariance ellipses visualization active");
            break;
            
        case DEBUG_MODE_EIGENVALUE_VISUALIZATION:
            // Visualize eigenvalues as colors
            debug_log_info("Eigenvalue visualization active");
            break;
            
        case DEBUG_MODE_ATLAS_PREVIEW:
            // Show texture atlas preview
            debug_log_info("Atlas preview active");
            break;
            
        case DEBUG_MODE_MEMORY_USAGE:
            // Show memory usage overlay
            debug_log_info("Memory usage visualization active");
            break;
    }
}

/*
 * Render performance overlay on screen
 */
void render_performance_overlay(void) {
    // This would render text overlay on screen showing:
    // - Frame time and FPS
    // - VU/DMA/GS timing breakdown
    // - Splat count and culling statistics
    // - Memory usage
    
    float ms_total = cycles_to_ms(frame_profile.total_frame_time);
    float fps = (ms_total > 0) ? (1000.0f / ms_total) : 0.0f;
    
    // For now, just log the data (real implementation would draw on screen)
    debug_log_info("PERF: %.1f FPS | %d splats | VU: %.1fms | GS: %.1fms", 
                   fps, frame_profile.splats_processed,
                   cycles_to_ms(frame_profile.vu_execute_time),
                   cycles_to_ms(frame_profile.gs_render_time));
}

/*
 * Enable/disable profiling
 */
void profiling_set_enabled(int enabled) {
    profiling_enabled = enabled;
    debug_log_info("Profiling %s", enabled ? "enabled" : "disabled");
}

/*
 * Check if profiling is enabled
 */
int profiling_is_enabled(void) {
    return profiling_enabled;
}

/*
 * Get performance statistics summary
 */
void profiling_get_stats_summary(float* avg_fps, float* avg_frame_time, 
                                u32* total_frames, u32* avg_splats) {
    if (avg_fps) {
        float ms_total = cycles_to_ms(frame_profile.total_frame_time);
        *avg_fps = (ms_total > 0) ? (1000.0f / ms_total) : 0.0f;
    }
    
    if (avg_frame_time) {
        *avg_frame_time = cycles_to_ms(frame_profile.total_frame_time);
    }
    
    if (total_frames) {
        *total_frames = frame_profile.frame_number;
    }
    
    if (avg_splats) {
        *avg_splats = frame_profile.splats_processed;
    }
}

/*
 * Reset profiling statistics
 */
void profiling_reset_stats(void) {
    memset(&frame_profile, 0, sizeof(frame_profile));
    debug_log_info("Profiling statistics reset");
}

/*
 * Cleanup profiling system
 */
void profiling_system_cleanup(void) {
    profiling_enabled = 0;
    current_debug_mode = DEBUG_MODE_NORMAL;
    memset(&frame_profile, 0, sizeof(frame_profile));
    debug_log_info("Profiling system cleaned up");
}

// Additional functions required by main_complete.c
void profiling_begin_frame(void) {
    profile_frame_start();
}

void profiling_end_frame(void) {
    // Calculate frame end time
    frame_profile.total_frame_time = get_cpu_cycles_64() - frame_profile.frame_start_time;
    frame_profile.frame_number++;
}

float profiling_get_frame_time(void) {
    return cycles_to_ms(frame_profile.total_frame_time);
}

float profiling_get_fps(void) {
    float frame_time = profiling_get_frame_time();
    return (frame_time > 0.0f) ? (1000.0f / frame_time) : 0.0f;
}

// Additional profiling functions for compatibility
int profiling_init(void) {
    return profiling_system_init();
}

void profiling_start_timer(const char* name) {
    // Simple implementation - could be enhanced with named timers
    if (strcmp(name, "frame") == 0) {
        profile_frame_start();
    } else if (strcmp(name, "dma") == 0) {
        profile_dma_upload_start();
    } else if (strcmp(name, "vu") == 0) {
        profile_vu_execute_start();
    } else if (strcmp(name, "gs") == 0) {
        profile_gs_render_start();
    }
}

void profiling_end_timer(const char* name) {
    // Simple implementation - could be enhanced with named timers
    if (strcmp(name, "frame") == 0) {
        profile_frame_end();
    } else if (strcmp(name, "dma") == 0) {
        profile_dma_upload_end();
    } else if (strcmp(name, "vu") == 0) {
        profile_vu_execute_end();
    } else if (strcmp(name, "gs") == 0) {
        profile_gs_render_end();
    }
}

void profiling_print_results(void) {
    float avg_frame_time, current_fps;
    u32 total_frames, avg_splats;
    profiling_get_stats_summary(&avg_frame_time, &current_fps, &total_frames, &avg_splats);
    
    printf("PROFILING RESULTS:\n");
    printf("  Frame Time: %.2f ms\n", avg_frame_time);
    printf("  FPS: %.1f\n", current_fps);
    printf("  Total Frames: %u\n", total_frames);
    printf("  Avg Splats: %u\n", avg_splats);
}

void profiling_get_data(FrameProfileData* data) {
    profile_get_frame_data(data);
}