/*
 * SPLATSTORM X - Optimized Memory Management Implementation
 * 64-byte aligned allocations and scratchpad usage
 */

#include <tamtypes.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include "splatstorm_x.h"
#include "memory_optimized.h"

// Scratchpad state
static u32 scratchpad_offset = 0;

/*
 * Allocate optimized splat array with 64-byte alignment
 * Ensures optimal cache performance and DMA efficiency
 */
PackedSplat* allocate_splat_array_optimized(int count) {
    if (count <= 0 || count > 65536) {
        debug_log_error("Invalid splat count: %d", count);
        return NULL;
    }
    
    size_t total_size = count * sizeof(PackedSplat);
    
    // 64-byte alignment for cache line optimization
    PackedSplat* splats = (PackedSplat*)memalign(64, total_size);
    if (!splats) {
        debug_log_error("Failed to allocate splat array for %d splats", count);
        return NULL;
    }
    
    // Clear and ensure cache coherency
    memset(splats, 0, total_size);
    SyncDCache(splats, (void*)((u32)splats + total_size));
    
    debug_log_info("Allocated optimized splat array: %d splats (%d bytes) at 0x%08x", 
                   count, total_size, (u32)splats);
    
    return splats;
}

/*
 * Free optimized splat array
 */
void free_splat_array_optimized(PackedSplat* splats) {
    if (splats) {
        free(splats);
        debug_log_info("Freed optimized splat array at 0x%08x", (u32)splats);
    }
}

/*
 * Allocate VU buffer from scratchpad if possible
 * Falls back to regular allocation if scratchpad full
 */
void* allocate_vu_buffer(size_t size) {
    if (size == 0) {
        debug_log_error("Invalid VU buffer size: 0");
        return NULL;
    }
    
    // Align to 16 bytes for VU processing
    size_t aligned_size = (size + 15) & ~15;
    
    // Try scratchpad first
    if (scratchpad_offset + aligned_size <= SCRATCHPAD_SIZE) {
        void* ptr = (void*)(SCRATCHPAD_BASE + scratchpad_offset);
        scratchpad_offset += aligned_size;
        
        debug_log_info("VU buffer allocated from scratchpad: %d bytes at 0x%08x", 
                       aligned_size, (u32)ptr);
        return ptr;
    }
    
    // Fallback to regular allocation with alignment
    void* ptr = memalign(16, aligned_size);
    if (ptr) {
        debug_log_info("VU buffer allocated from EE RAM: %d bytes at 0x%08x", 
                       aligned_size, (u32)ptr);
    } else {
        debug_log_error("Failed to allocate VU buffer of %d bytes", aligned_size);
    }
    
    return ptr;
}

/*
 * Allocate DMA-safe buffer with proper alignment
 * Ensures 128-byte alignment for optimal DMA performance
 */
void* allocate_dma_buffer_aligned(size_t size) {
    if (size == 0) {
        debug_log_error("Invalid DMA buffer size: 0");
        return NULL;
    }
    
    // DMA buffers need 128-byte alignment for best performance
    size_t aligned_size = (size + 127) & ~127;
    void* buffer = memalign(128, aligned_size);
    
    if (buffer) {
        // Clear buffer and ensure cache coherency
        memset(buffer, 0, aligned_size);
        SyncDCache(buffer, (void*)((u32)buffer + aligned_size));
        
        debug_log_info("DMA buffer allocated: %d bytes at 0x%08x", aligned_size, (u32)buffer);
    } else {
        debug_log_error("Failed to allocate DMA buffer of %d bytes", aligned_size);
    }
    
    return buffer;
}

/*
 * Free DMA buffer
 */
void free_dma_buffer_aligned(void* buffer) {
    if (buffer) {
        free(buffer);
        debug_log_info("Freed DMA buffer at 0x%08x", (u32)buffer);
    }
}

/*
 * Reset scratchpad allocator
 * Allows reuse of scratchpad memory (useful for per-frame allocations)
 */
void scratchpad_reset(void) {
    scratchpad_offset = 0;
    debug_log_info("Scratchpad allocator reset");
}

/*
 * Allocate from scratchpad memory
 * Ultra-fast allocation for temporary data
 */
void* scratchpad_alloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align to 16 bytes
    size_t aligned_size = (size + 15) & ~15;
    
    if (scratchpad_offset + aligned_size > SCRATCHPAD_SIZE) {
        debug_log_error("Scratchpad full: requested %d bytes, available %d bytes", 
                       aligned_size, SCRATCHPAD_SIZE - scratchpad_offset);
        return NULL;
    }
    
    void* ptr = (void*)(SCRATCHPAD_BASE + scratchpad_offset);
    scratchpad_offset += aligned_size;
    
    debug_log_info("Scratchpad allocation: %d bytes at 0x%08x", aligned_size, (u32)ptr);
    return ptr;
}

/*
 * Get available scratchpad space
 */
int scratchpad_available(void) {
    return SCRATCHPAD_SIZE - scratchpad_offset;
}

/*
 * Ensure cache coherency for DMA operations
 */
void ensure_cache_coherency(void* ptr, size_t size) {
    if (ptr && size > 0) {
        SyncDCache(ptr, (void*)((u32)ptr + size));
    }
}

/*
 * Flush all caches
 */
void flush_all_caches(void) {
    FlushCache(0);  // Instruction cache
    FlushCache(2);  // Data cache
    debug_log_info("All caches flushed");
}