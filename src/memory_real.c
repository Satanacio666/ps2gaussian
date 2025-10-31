/*
 * SPLATSTORM X - Real Memory Management Implementation
 * PS2-optimized memory management with scratchpad support
 * Based on your technical specifications and ps2dev/ps2dev:latest
 */

#include <tamtypes.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include "splatstorm_x.h"

// Scratchpad memory constants
#define SCRATCHPAD_BASE 0x70000000
#define SCRATCHPAD_SIZE (16 * 1024)  // 16KB

// Memory management state
static u32 scratchpad_offset = 0;
static int memory_system_initialized = 0;
static size_t total_allocated = 0;
static size_t scratchpad_allocated = 0;

// Memory pool for large allocations
#define MEMORY_POOL_SIZE (2 * 1024 * 1024)  // 2MB pool
static void *memory_pool = NULL;
static size_t pool_offset = 0;

// memory_system_init function removed to avoid multiple definition conflict
// The implementation is available in memory_system_complete.c

// REMOVED: Duplicate function - using UTMOST UPGRADED version in splatstorm_core_system.c

/*
 * Scratchpad memory allocation for critical data
 * Uses ultra-fast 16KB scratchpad for VU buffers and temp data
 */
void* splatstorm_malloc_scratchpad(size_t size) {
    if (!memory_system_initialized) {
        debug_log_error("Memory system not initialized");
        return NULL;
    }
    
    if (size == 0) {
        debug_log_error("Invalid scratchpad allocation size: 0");
        return NULL;
    }
    
    // Align to 16 bytes
    size_t aligned_size = (size + 15) & ~15;
    
    // Check if we have space in scratchpad
    if (scratchpad_offset + aligned_size > SCRATCHPAD_SIZE) {
        debug_log_error("Scratchpad full: requested %d bytes, available %d bytes", 
                       aligned_size, SCRATCHPAD_SIZE - scratchpad_offset);
        
        // Fall back to regular allocation
        return splatstorm_malloc(size);
    }
    
    // Allocate from scratchpad
    void *ptr = (void*)(SCRATCHPAD_BASE + scratchpad_offset);
    scratchpad_offset += aligned_size;
    scratchpad_allocated += aligned_size;
    
    debug_log_info("Scratchpad allocation: %d bytes at 0x%08x", aligned_size, (u32)ptr);
    return ptr;
}

// REMOVED: Duplicate function - using UTMOST UPGRADED version in splatstorm_core_system.c

/*
 * Reset scratchpad allocator
 * Allows reuse of scratchpad memory
 */
void splatstorm_free_scratchpad(void* ptr) {
    if (!ptr) return;
    
    u32 addr = (u32)ptr;
    
    // Check if it's actually in scratchpad
    if (addr < SCRATCHPAD_BASE || addr >= SCRATCHPAD_BASE + SCRATCHPAD_SIZE) {
        debug_log_error("Pointer 0x%08x is not in scratchpad", addr);
        return;
    }
    
    // For simplicity, reset entire scratchpad if freeing the first allocation
    if (addr == SCRATCHPAD_BASE) {
        scratchpad_offset = 0;
        scratchpad_allocated = 0;
        debug_log_info("Scratchpad reset");
    } else {
        debug_log_info("Cannot free individual scratchpad allocation (use reset)");
    }
}

// scratchpad_reset function removed to avoid multiple definition conflict
// The implementation is available in memory_system_complete.c

/*
 * Allocate memory for Gaussian splat data
 * Optimized for splat storage with proper alignment
 */
GaussianSplat3D* allocate_splat_array(int count) {
    if (count <= 0) {
        debug_log_error("Invalid splat count: %d", count);
        return NULL;
    }
    
    size_t size = count * sizeof(GaussianSplat3D);
    GaussianSplat3D *splats = (GaussianSplat3D*)splatstorm_malloc(size);
    
    if (splats) {
        // Clear the array
        memset(splats, 0, size);
        debug_log_info("Allocated splat array: %d splats (%d bytes) at 0x%08x", 
                       count, size, (u32)splats);
    } else {
        debug_log_error("Failed to allocate splat array for %d splats", count);
    }
    
    return splats;
}

/*
 * Free splat array
 */
void free_splat_array(GaussianSplat3D* splats) {
    if (splats) {
        splatstorm_free(splats);
        debug_log_info("Freed splat array at 0x%08x", (u32)splats);
    }
}

/*
 * Allocate DMA-safe buffer
 * Ensures proper alignment and cache coherency
 */
void* allocate_dma_buffer(size_t size) {
    if (size == 0) {
        debug_log_error("Invalid DMA buffer size: 0");
        return NULL;
    }
    
    // DMA buffers must be 16-byte aligned and cache-line aligned
    size_t aligned_size = (size + 63) & ~63;  // 64-byte alignment for cache
    void *buffer = memalign(64, aligned_size);
    
    if (buffer) {
        // Clear buffer
        memset(buffer, 0, aligned_size);
        
        // Ensure cache coherency
        FlushCache(0);
        SyncDCache(buffer, (char*)buffer + aligned_size);
        
        debug_log_info("Allocated DMA buffer: %d bytes at 0x%08x", aligned_size, (u32)buffer);
    } else {
        debug_log_error("Failed to allocate DMA buffer of %d bytes", aligned_size);
    }
    
    return buffer;
}

/*
 * Free DMA buffer
 */
void free_dma_buffer(void* buffer) {
    if (buffer) {
        free(buffer);
        debug_log_info("Freed DMA buffer at 0x%08x", (u32)buffer);
    }
}

/*
 * Get memory usage statistics
 */
void memory_get_stats(MemoryStats *stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(MemoryStats));
    
    stats->total_allocated = total_allocated;
    stats->scratchpad_allocated = scratchpad_allocated;
    stats->scratchpad_available = SCRATCHPAD_SIZE - scratchpad_offset;
    stats->pool_used = pool_offset;
    stats->pool_available = MEMORY_POOL_SIZE - pool_offset;
    stats->system_initialized = memory_system_initialized;
}

/*
 * Print memory usage information
 */
void memory_print_stats(void) {
    MemoryStats stats;
    memory_get_stats(&stats);
    
    debug_log_info("=== Memory Statistics ===");
    debug_log_info("System initialized: %s", stats.system_initialized ? "Yes" : "No");
    debug_log_info("Total allocated: %d bytes", stats.total_allocated);
    debug_log_info("Scratchpad used: %d / %d bytes", 
                   stats.scratchpad_allocated, SCRATCHPAD_SIZE);
    debug_log_info("Memory pool used: %d / %d bytes", 
                   stats.pool_used, MEMORY_POOL_SIZE);
    debug_log_info("========================");
}

// memory_system_cleanup is implemented in memory_system_complete.c