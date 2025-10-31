/*
 * SPLATSTORM X - Optimized Depth Sorting Implementation
 * Bucket sort with temporal coherence for efficient splat sorting
 */

#include <tamtypes.h>
#include <string.h>
#include <stdlib.h>
#include "splatstorm_x.h"
#include "gaussian_types.h"
#include "splatstorm_debug.h"
#include "memory_optimized.h"

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// Forward declarations
static void radix_sort_splats(splat_t* splats, int* sorted_indices, int count);
static void quick_sort_splats_by_depth(splat_t* splats, int* sorted_indices, int start, int end);

typedef struct {
    PackedSplat* splats;
    float* depths;
    int* bucket_indices;
    int count;
    u64 last_sort_frame;
    int camera_moved;
} SortingContext;

static SortingContext sorting_context = {0};
static u64 current_frame = 0;

/*
 * Initialize sorting system
 */
int sorting_system_init(PackedSplat* splats, int count) {
    sorting_context.splats = splats;
    sorting_context.count = count;
    sorting_context.last_sort_frame = 0;
    sorting_context.camera_moved = 1;  // Force initial sort
    
    // Allocate depth array
    sorting_context.depths = (float*)allocate_vu_buffer(count * sizeof(float));
    if (!sorting_context.depths) {
        debug_log_error("Failed to allocate depth array");
        return -1;
    }
    
    // Allocate bucket indices
    sorting_context.bucket_indices = (int*)allocate_vu_buffer(count * sizeof(int));
    if (!sorting_context.bucket_indices) {
        debug_log_error("Failed to allocate bucket indices");
        return -2;
    }
    
    debug_log_info("Sorting system initialized for %d splats", count);
    return 0;
}

/*
 * Bucket sort with temporal coherence optimization
 */
void bucket_sort_splats_optimized(void) {
    static int buckets[NUM_DEPTH_BUCKETS + 1];
    
    current_frame++;
    
    // Skip sorting if camera hasn't moved significantly
    if (sorting_context.last_sort_frame == current_frame - 1 && !sorting_context.camera_moved) {
        debug_log_info("Skipping sort - temporal coherence");
        return;
    }
    
    debug_log_info("Performing bucket sort for %d splats", sorting_context.count);
    
    // Clear bucket counters
    memset(buckets, 0, sizeof(buckets));
    
    // Extract depths and count splats per bucket
    float min_depth = 999999.0f, max_depth = -999999.0f;
    for (int i = 0; i < sorting_context.count; i++) {
        float depth = sorting_context.splats[i].pos_color[2];  // Z coordinate
        sorting_context.depths[i] = depth;
        
        if (depth < min_depth) min_depth = depth;
        if (depth > max_depth) max_depth = depth;
    }
    
    // Normalize depths to bucket range
    float depth_range = max_depth - min_depth;
    if (depth_range < 0.001f) depth_range = 0.001f;  // Avoid division by zero
    
    // Count splats per bucket (back-to-front for alpha blending)
    for (int i = 0; i < sorting_context.count; i++) {
        float normalized_depth = (sorting_context.depths[i] - min_depth) / depth_range;
        int bucket = (int)(normalized_depth * NUM_DEPTH_BUCKETS);
        bucket = CLAMP(bucket, 0, NUM_DEPTH_BUCKETS - 1);
        sorting_context.bucket_indices[i] = bucket;
        buckets[bucket + 1]++;
    }
    
    // Prefix sum for bucket boundaries
    for (int i = 1; i <= NUM_DEPTH_BUCKETS; i++) {
        buckets[i] += buckets[i - 1];
    }
    
    // Allocate temporary buffer (use scratchpad if possible)
    PackedSplat* temp = (PackedSplat*)allocate_vu_buffer(sorting_context.count * sizeof(PackedSplat));
    if (!temp) {
        debug_log_error("Failed to allocate temporary sort buffer");
        return;
    }
    
    // Distribute splats into buckets (back-to-front order for alpha blending)
    for (int i = sorting_context.count - 1; i >= 0; i--) {
        int bucket = sorting_context.bucket_indices[i];
        int index = buckets[bucket + 1] - 1;
        temp[index] = sorting_context.splats[i];
        buckets[bucket + 1]--;
    }
    
    // Copy back to original array
    memcpy(sorting_context.splats, temp, sorting_context.count * sizeof(PackedSplat));
    
    sorting_context.last_sort_frame = current_frame;
    sorting_context.camera_moved = 0;  // Reset camera moved flag
    
    debug_log_info("Bucket sort completed: %d buckets, range %.3f to %.3f", 
                   NUM_DEPTH_BUCKETS, min_depth, max_depth);
}

/*
 * Signal that camera has moved (forces resort)
 */
void sorting_camera_moved(void) {
    sorting_context.camera_moved = 1;
    debug_log_info("Camera movement detected - will resort next frame");
}

// camera_moved_significantly function is implemented in tile_rasterizer_complete.c

/*
 * Get sorting statistics
 */
void get_sorting_stats(int* last_sort_frame, int* buckets_used, float* sort_time_ms) {
    if (last_sort_frame) *last_sort_frame = sorting_context.last_sort_frame;
    if (buckets_used) *buckets_used = NUM_DEPTH_BUCKETS;
    if (sort_time_ms) *sort_time_ms = 2.5f;  // Estimated sort time
}

/*
 * Cleanup sorting system
 */
void sorting_system_cleanup(void) {
    // Note: Memory allocated via allocate_vu_buffer() is managed by memory system
    memset(&sorting_context, 0, sizeof(sorting_context));
    debug_log_info("Sorting system cleaned up");
}

/*
 * Depth sort splats based on camera position
 * Matches header signature: void depth_sort_splats(splat_t* splats, int count, int* sorted_indices, int mode);
 */
void depth_sort_splats(splat_t* splats, int count, int* sorted_indices, int mode) {
    debug_log_info("Depth sorting %d splats with mode %d", count, mode);
    
    if (!splats || count <= 0 || !sorted_indices) {
        debug_log_error("Invalid parameters for depth sorting");
        return;
    }
    
    // Initialize sorted indices array
    for (int i = 0; i < count; i++) {
        sorted_indices[i] = i;
    }
    
    // Choose sorting algorithm based on mode
    if (mode == 0) {
        // Quick sort mode
        quick_sort_splats_by_depth(splats, sorted_indices, 0, count - 1);
    } else {
        // Radix sort mode
        radix_sort_splats(splats, sorted_indices, count);
    }
    
    debug_log_info("Depth sorting complete");
}

/*
 * Radix sort implementation for splats (optimized for PS2)
 */
static void radix_sort_splats(splat_t* splats, int* sorted_indices, int count) {
    debug_log_info("Radix sorting %d splats", count);
    
    if (!splats || !sorted_indices || count <= 0) {
        debug_log_error("Invalid parameters for radix sort");
        return;
    }
    
    // Allocate temporary array for radix sort
    int* temp_indices = (int*)malloc(count * sizeof(int));
    if (!temp_indices) {
        debug_log_error("Failed to allocate temporary array for radix sort");
        return;
    }
    
    // Convert depths to integers for radix sort (multiply by 1000 for precision)
    u32* depth_keys = (u32*)malloc(count * sizeof(u32));
    if (!depth_keys) {
        debug_log_error("Failed to allocate depth keys array");
        free(temp_indices);
        return;
    }
    
    // Calculate depth for each splat (using Z position)
    for (int i = 0; i < count; i++) {
        depth_keys[i] = (u32)(splats[sorted_indices[i]].pos[2] * 1000.0f);
    }
    
    // Radix sort by 8-bit digits (4 passes for 32-bit keys)
    for (int shift = 0; shift < 32; shift += 8) {
        int bucket_counts[256] = {0};
        int bucket_offsets[256];
        
        // Count occurrences of each digit
        for (int i = 0; i < count; i++) {
            u8 digit = (depth_keys[i] >> shift) & 0xFF;
            bucket_counts[digit]++;
        }
        
        // Calculate bucket offsets
        bucket_offsets[0] = 0;
        for (int i = 1; i < 256; i++) {
            bucket_offsets[i] = bucket_offsets[i-1] + bucket_counts[i-1];
        }
        
        // Distribute elements into buckets
        for (int i = 0; i < count; i++) {
            u8 digit = (depth_keys[i] >> shift) & 0xFF;
            int pos = bucket_offsets[digit]++;
            temp_indices[pos] = sorted_indices[i];
        }
        
        // Copy back to original array
        memcpy(sorted_indices, temp_indices, count * sizeof(int));
    }
    
    // Clean up temporary arrays
    free(temp_indices);
    free(depth_keys);
    
    debug_log_info("Radix sort complete");
}

/*
 * Quick sort implementation for splats by depth
 */
static void quick_sort_splats_by_depth(splat_t* splats, int* sorted_indices, int start, int end) {
    if (start >= end) return;
    
    // Choose pivot (middle element)
    int pivot_idx = start + (end - start) / 2;
    // Calculate depth from Z position (assuming camera is looking down -Z)
    float pivot_depth = splats[sorted_indices[pivot_idx]].pos[2];
    
    // Swap pivot to end
    int temp = sorted_indices[pivot_idx];
    sorted_indices[pivot_idx] = sorted_indices[end];
    sorted_indices[end] = temp;
    
    // Partition around pivot
    int store_idx = start;
    for (int i = start; i < end; i++) {
        float current_depth = splats[sorted_indices[i]].pos[2];
        if (current_depth > pivot_depth) {  // Sort back-to-front for alpha blending
            int temp_idx = sorted_indices[i];
            sorted_indices[i] = sorted_indices[store_idx];
            sorted_indices[store_idx] = temp_idx;
            store_idx++;
        }
    }
    
    // Move pivot to final position
    int temp_idx = sorted_indices[store_idx];
    sorted_indices[store_idx] = sorted_indices[end];
    sorted_indices[end] = temp_idx;
    
    // Recursively sort partitions
    quick_sort_splats_by_depth(splats, sorted_indices, start, store_idx - 1);
    quick_sort_splats_by_depth(splats, sorted_indices, store_idx + 1, end);
}

/*
 * Comparison function for splat depth (for qsort compatibility)
 */
int compare_splat_depth(const void* a, const void* b) {
    const splat_t* splat_a = (const splat_t*)a;
    const splat_t* splat_b = (const splat_t*)b;
    
    // Sort back-to-front for proper alpha blending (using Z position)
    float depth_a = splat_a->pos[2];
    float depth_b = splat_b->pos[2];
    if (depth_a > depth_b) return -1;
    if (depth_a < depth_b) return 1;
    return 0;
}