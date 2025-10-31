/*
 * SPLATSTORM X - Debug & Profiling System
 * Comprehensive debugging with cycle counting and logging
 */

#ifndef SPLATSTORM_DEBUG_H
#define SPLATSTORM_DEBUG_H

    // COMPLETE IMPLEMENTATION - Full functionality
#include <tamtypes.h>


// Debug Configuration
#define DEBUG_ENABLED               1
#define DEBUG_LOG_BUFFER_SIZE       (64 * 1024)
#define DEBUG_MAX_PROFILES          32
#define DEBUG_MAX_LOG_ENTRIES       1024

// Debug Levels
#define DEBUG_LEVEL_ERROR           0
#define DEBUG_LEVEL_WARNING         1
#define DEBUG_LEVEL_INFO            2
#define DEBUG_LEVEL_VERBOSE         3

// Profile Entry
typedef struct {
    char name[32];
    u64 start_time;
    u64 total_time;
    u32 call_count;
    u32 cycles;
    u32 active;
} profile_entry_t;

// Debug Statistics
typedef struct {
    // Performance
    u32 fps_current;
    u32 fps_average;
    u32 fps_min;
    u32 fps_max;
    
    // Memory
    u32 memory_ee_used;
    u32 memory_ee_peak;
    u32 memory_vram_used;
    u32 memory_vram_peak;
    
    // Rendering
    u32 splats_total;
    u32 splats_visible;
    u32 splats_culled;
    u32 draw_calls;
    
    // Timing (in cycles)
    u32 cycles_vu0;
    u32 cycles_vu1;
    u32 cycles_gs;
    u32 cycles_dma;
    u32 cycles_total;
    
    // Errors
    u32 error_count;
    u32 warning_count;
    
} debug_stats_t;

// Debug Functions
void debug_init(void);
void debug_shutdown(void);
void debug_set_level(u32 level);

// Logging
void debug_log(u32 level, const char* format, ...);
void debug_log_error(const char* format, ...);
void debug_log_warning(const char* format, ...);
void debug_log_info(const char* format, ...);
void debug_log_verbose(const char* format, ...);

// Profiling
void debug_profile_begin(const char* name);
void debug_profile_end(const char* name);
void debug_profile_reset(void);
void debug_profile_dump(void);

// Performance Monitoring
void debug_update_fps(float fps);
void debug_update_memory(u32 ee_used, u32 vram_used);
void debug_update_rendering(u32 total, u32 visible, u32 culled);
void debug_update_cycles(u32 vu0, u32 vu1, u32 gs, u32 dma);

// On-Screen Display
void debug_draw_overlay(void);
void debug_draw_graph(const char* name, float* values, u32 count);
void debug_draw_text(u32 x, u32 y, const char* text);

// Memory Debugging
void debug_check_stack_overflow(void);
void debug_check_heap_corruption(void);
void debug_dump_memory_map(void);

// VU Debugging
void debug_dump_vu0_state(void);
void debug_dump_vu1_state(void);
void debug_verify_vu_microcode(void);

// File Output
void debug_save_log(const char* filename);
void debug_save_profile_data(const char* filename);
void debug_save_frame_capture(const char* filename);

// Assertions
#define DEBUG_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            debug_log_error("ASSERTION FAILED: %s at %s:%d", #condition, __FILE__, __LINE__); \
            splatstorm_emergency_shutdown(); \
        } \
    } while(0)

#define DEBUG_VERIFY(condition, message) \
    do { \
        if (!(condition)) { \
            debug_log_error("VERIFICATION FAILED: %s - %s at %s:%d", message, #condition, __FILE__, __LINE__); \
            return -1; \
        } \
    } while(0)

// Profiling Macros
#define PROFILE_BEGIN(name) debug_profile_begin(name)
#define PROFILE_END(name) debug_profile_end(name)

#define PROFILE_SCOPE(name) \
    debug_profile_begin(name); \
    for(int _prof_i = 1; _prof_i; _prof_i = 0, debug_profile_end(name))

// Global Debug State
extern debug_stats_t g_debug_stats;
extern u32 g_debug_level;

#endif // SPLATSTORM_DEBUG_H
