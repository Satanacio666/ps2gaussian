/*
 * SPLATSTORM X - Professional DMA System
 * Based on AthenaEnv's VIF packet management
 * Replaces DMA stubs with real packet transmission
 */

#include "splatstorm_x.h"
#include "splatstorm_debug.h"
#include "vif_macros_extended.h"
#include <dma.h>
#include <packet.h>
#include <dma_tags.h>
#include <vif_codes.h>
#include <malloc.h>
#include <string.h>

// VIF command constants (PS2SDK compatibility)
// Using PS2SDK vif_codes.h constants
// VIF_UNPACK_V4_32: M=0, VN=3 (V4), VL=0 (32-bit) = 0x6C
#define VIF_UNPACK_V4_32 VIF_CMD_UNPACK(0, 3, 0)

// DMA packet management (from AthenaEnv/src/vif.c)
void splatstorm_vif_send_packet(void* packet, u32 vif_channel) {
    if (!packet) {
        debug_log_error("VIF DMA: Invalid packet pointer");
        return;
    }
    
    debug_log_verbose("VIF DMA: Sending packet to channel %d", vif_channel);
    
    // Wait for DMA channel to be ready - Direct PS2SDK implementation
    dma_channel_wait(vif_channel, 0);
    
    // Flush cache to ensure data coherency
    FlushCache(0);
    
    // Send DMA chain (mask address to 28-bit physical) - Direct PS2SDK implementation
    dma_channel_send_chain(vif_channel, (void*)((u32)packet & 0x0FFFFFFF), 0, 0, 0);
    
    debug_log_verbose("VIF DMA: Packet sent successfully");
}

void* splatstorm_vif_create_packet(u32 size) {
    if (size == 0) {
        debug_log_error("VIF DMA: Invalid packet size: 0");
        return NULL;
    }
    
    // Allocate 128-byte aligned memory for DMA (critical for PS2)
    void* packet = memalign(128, size * 16);
    if (!packet) {
        debug_log_error("VIF DMA: Failed to allocate %d bytes for packet", size * 16);
        return NULL;
    }
    
    // Clear packet memory
    memset(packet, 0, size * 16);
    
    debug_log_verbose("VIF DMA: Created packet of %d quadwords (%d bytes) at %p", 
                     size, size * 16, packet);
    
    return packet;
}

void splatstorm_vif_destroy_packet(void* packet) {
    if (packet) {
        free(packet);
        debug_log_verbose("VIF DMA: Packet destroyed at %p", packet);
    }
}

// Calculate packet size for VU program upload
static inline u32 get_packet_size_for_program(u32* start, u32* end) {
    if (!start || !end || start >= end) {
        return 0;
    }
    
    // Count instructions (each VU instruction is 64-bit = 2 u32s)
    u32 count = (end - start) / 2;
    if (count & 1) {
        count++; // Round up to even number
    }
    
    // Calculate number of 256-instruction chunks needed
    return (count >> 8) + 1;
}

// Professional VU1 microcode upload (replaces stub implementation)
void splatstorm_vu1_upload_microcode(u32* start, u32* end) {
    if (!start || !end || start >= end) {
        debug_log_error("VIF DMA: Invalid VU1 microcode pointers");
        return;
    }
    
    debug_log_info("VIF DMA: Uploading VU1 microcode from %p to %p", start, end);
    
    // Calculate packet size (includes space for end tag)
    u32 packet_size = get_packet_size_for_program(start, end) + 1;
    u64* p_store;
    u64* p_data = p_store = (u64*)splatstorm_vif_create_packet(packet_size);
    
    if (!p_data) {
        debug_log_error("VIF DMA: Failed to create packet for VU1 upload");
        return;
    }
    
    // Calculate instruction count
    u32 dest = 0;
    u32 count = (end - start) / 2;
    if (count & 1) {
        count++;
    }
    
    u32* l_start = start;
    
    debug_log_info("VIF DMA: Uploading %d VU1 instructions in chunks", count);
    
    // Upload in 256-instruction chunks (VU1 limitation)
    while (count > 0) {
        u16 curr_count = count > 256 ? 256 : count;
        
        debug_log_verbose("VIF DMA: Uploading chunk: %d instructions at dest %d", 
                         curr_count, dest);
        
        // DMA tag: REF to microcode data
        *p_data++ = DMA_TAG(curr_count / 2, 0, DMA_REF, 0, (const u128*)l_start, 0);
        
        // VIF codes: NOP + MPG (MicroProgram)
        *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                    (u64)VIF_CODE(dest, curr_count & 0xFF, VIF_CMD_MPG, 0) << 32);
        
        l_start += curr_count * 2;
        count -= curr_count;
        dest += curr_count;
    }
    
    // End tag
    *p_data++ = DMA_TAG(0, 0, DMA_END, 0, 0, 0);
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, 0, VIF_CMD_NOP, 0) << 32);
    
    // Send packet to VIF1
    splatstorm_vif_send_packet(p_store, DMA_CHANNEL_VIF1);
    splatstorm_vif_destroy_packet(p_store);
    
    debug_log_info("VIF DMA: VU1 microcode upload complete");
}

// Professional VU0 microcode upload
void splatstorm_vu0_upload_microcode(u32* start, u32* end) {
    if (!start || !end || start >= end) {
        debug_log_error("VIF DMA: Invalid VU0 microcode pointers");
        return;
    }
    
    debug_log_info("VIF DMA: Uploading VU0 microcode from %p to %p", start, end);
    
    // VU0 has smaller instruction memory (4KB = 512 instructions)
    u32 count = (end - start) / 2;
    if (count > 512) {
        debug_log_warning("VIF DMA: VU0 program too large (%d instructions), truncating to 512", count);
        count = 512;
        end = start + (512 * 2);
    }
    
    // Create packet for VU0 upload
    u32 packet_size = 3; // Header + data ref + end tag
    u64* p_store;
    u64* p_data = p_store = (u64*)splatstorm_vif_create_packet(packet_size);
    
    if (!p_data) {
        debug_log_error("VIF DMA: Failed to create packet for VU0 upload");
        return;
    }
    
    // DMA tag: REF to microcode data
    *p_data++ = DMA_TAG(count / 2, 0, DMA_REF, 0, (const u128*)start, 0);
    
    // VIF codes: NOP + MPG for VU0
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, count & 0xFF, VIF_CMD_MPG, 0) << 32);
    
    // End tag
    *p_data++ = DMA_TAG(0, 0, DMA_END, 0, 0, 0);
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, 0, VIF_CMD_NOP, 0) << 32);
    
    // Send packet to VIF0 (note: VU0 uses different channel)
    splatstorm_vif_send_packet(p_store, DMA_CHANNEL_VIF0);
    splatstorm_vif_destroy_packet(p_store);
    
    debug_log_info("VIF DMA: VU0 microcode upload complete (%d instructions)", count);
}

// VU1 double buffer settings (from AthenaEnv)
void splatstorm_vu1_set_double_buffer_settings(u32 base, u32 offset) {
    debug_log_info("VIF DMA: Setting VU1 double buffer - base: %d, offset: %d", base, offset);
    
    u64* p_data;
    u64* p_store;
    p_data = p_store = (u64*)splatstorm_vif_create_packet(2);
    
    if (!p_data) {
        debug_log_error("VIF DMA: Failed to create packet for double buffer settings");
        return;
    }
    
    // DMA tag: CNT (count)
    *p_data++ = DMA_TAG(0, 0, DMA_CNT, 0, 0, 0);
    
    // VIF codes: BASE + OFFSET
    *p_data++ = (VIF_CODE(base, 0, VIF_CMD_BASE, 0) | 
                (u64)VIF_CODE(offset, 0, VIF_CMD_OFFSET, 0) << 32);
    
    // End tag
    *p_data++ = DMA_TAG(0, 0, DMA_END, 0, 0, 0);
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, 0, VIF_CMD_NOP, 0) << 32);
    
    splatstorm_vif_send_packet(p_store, DMA_CHANNEL_VIF1);
    splatstorm_vif_destroy_packet(p_store);
    
    debug_log_info("VIF DMA: VU1 double buffer settings applied");
}

// Send data to VU1 data memory
void splatstorm_vu1_send_data(void* data, u32 size, u32 dest_addr) {
    if (!data || size == 0) {
        debug_log_error("VIF DMA: Invalid data parameters");
        return;
    }
    
    debug_log_verbose("VIF DMA: Sending %d bytes to VU1 data memory at address %d", size, dest_addr);
    
    // Calculate quadwords
    u32 qwords = (size + 15) / 16; // Round up to quadwords
    
    u64* p_data;
    u64* p_store;
    p_data = p_store = (u64*)splatstorm_vif_create_packet(qwords + 2);
    
    if (!p_data) {
        debug_log_error("VIF DMA: Failed to create packet for data transfer");
        return;
    }
    
    // DMA tag: REF to data
    *p_data++ = DMA_TAG(qwords, 0, DMA_REF, 0, (const u128*)data, 0);
    
    // VIF codes: NOP + UNPACK (data transfer)
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(dest_addr, qwords, VIF_UNPACK_V4_32, 0) << 32);
    
    // End tag
    *p_data++ = DMA_TAG(0, 0, DMA_END, 0, 0, 0);
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, 0, VIF_CMD_NOP, 0) << 32);
    
    splatstorm_vif_send_packet(p_store, DMA_CHANNEL_VIF1);
    splatstorm_vif_destroy_packet(p_store);
    
    debug_log_verbose("VIF DMA: Data transfer complete");
}

// Start VU1 program execution
void splatstorm_vu1_start_program(u32 start_addr) {
    debug_log_info("VIF DMA: Starting VU1 program at address %d", start_addr);
    
    u64* p_data;
    u64* p_store;
    p_data = p_store = (u64*)splatstorm_vif_create_packet(2);
    
    if (!p_data) {
        debug_log_error("VIF DMA: Failed to create packet for program start");
        return;
    }
    
    // DMA tag: CNT
    *p_data++ = DMA_TAG(0, 0, DMA_CNT, 0, 0, 0);
    
    // VIF codes: NOP + MSCAL (start microprogram)
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(start_addr, 0, VIF_CMD_MSCAL, 0) << 32);
    
    // End tag
    *p_data++ = DMA_TAG(0, 0, DMA_END, 0, 0, 0);
    *p_data++ = (VIF_CODE(0, 0, VIF_CMD_NOP, 0) | 
                (u64)VIF_CODE(0, 0, VIF_CMD_NOP, 0) << 32);
    
    splatstorm_vif_send_packet(p_store, DMA_CHANNEL_VIF1);
    splatstorm_vif_destroy_packet(p_store);
    
    debug_log_info("VIF DMA: VU1 program started");
}

// Wait for VU1 to finish
void splatstorm_vu1_wait_finish(void) {
    debug_log_verbose("VIF DMA: Waiting for VU1 to finish");
    
    // Wait for VIF1 DMA to complete - Direct PS2SDK implementation
    dma_channel_wait(DMA_CHANNEL_VIF1, 0);
    
    debug_log_verbose("VIF DMA: VU1 finished");
}

// Enhanced DMA chain sending (replaces stub)
void splatstorm_dma_send_chain(void* data, u32 size) {
    if (!data || size == 0) {
        debug_log_error("VIF DMA: Invalid chain parameters");
        return;
    }
    
    debug_log_info("VIF DMA: Sending DMA chain of %u bytes", size);
    
    // Ensure data is properly aligned
    if ((u32)data & 0xF) {
        debug_log_warning("VIF DMA: Data not 16-byte aligned, performance may be affected");
    }
    
    // Flush cache for data coherency
    FlushCache(0);
    
    // Send chain to GIF (Graphics Interface) - Direct PS2SDK implementation
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
    dma_channel_send_chain(DMA_CHANNEL_GIF, (void*)((u32)data & 0x0FFFFFFF), 0, 0, 0);
    
    debug_log_info("VIF DMA: DMA chain sent successfully");
}

// Build display list for splat rendering (replaces stub)
void splatstorm_dma_build_display_list(splat_t* splats, u32 count) {
    if (!splats || count == 0) {
        debug_log_warning("VIF DMA: Cannot build display list - invalid parameters");
        return;
    }
    
    debug_log_info("VIF DMA: Building display list for %u splats", count);
    
    // Use the real splat renderer with camera matrices
    if (camera_is_initialized()) {
        float* view_matrix = camera_get_view_matrix();
        float* proj_matrix = camera_get_proj_matrix();
        
        if (view_matrix && proj_matrix) {
            // Send matrices to VU1 first
            splatstorm_vu1_send_data(view_matrix, 16 * sizeof(float), 0x100);
            splatstorm_vu1_send_data(proj_matrix, 16 * sizeof(float), 0x110);
            
            // Send splat data
            u32 data_size = count * sizeof(splat_t);
            splatstorm_vu1_send_data(splats, data_size, 0x200);
            
            // Start VU1 rendering program
            splatstorm_vu1_start_program(0);
            
            // Wait for completion
            splatstorm_vu1_wait_finish();
            
            // Get rendering statistics
            u32 processed, visible, culled, pixels;
            float time_ms;
            splat_renderer_get_stats(&processed, &visible, &culled, &pixels, &time_ms);
            
            debug_log_info("VIF DMA: Rendered %u/%u splats (%u pixels) in %.2fms", 
                          visible, processed, pixels, time_ms);
        } else {
            debug_log_error("VIF DMA: Invalid camera matrices");
        }
    } else {
        debug_log_warning("VIF DMA: Camera not initialized - cannot render splats");
    }
}

// DMA system initialization
int splatstorm_dma_init(void) {
    debug_log_info("VIF DMA: Initializing professional DMA system");
    
    // Initialize DMA system with proper settings - Direct PS2SDK implementation
    dma_reset();
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF0, NULL, 0);
    
    debug_log_info("VIF DMA: DMA system initialized (GIF, VIF0, VIF1)");
    return 1;
}

// DMA system shutdown
void splatstorm_dma_shutdown(void) {
    debug_log_info("VIF DMA: Shutting down DMA system");
    
    // Wait for all channels to finish - Direct PS2SDK implementation
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
    dma_channel_wait(DMA_CHANNEL_VIF1, 0);
    dma_channel_wait(DMA_CHANNEL_VIF0, 0);
    
    debug_log_info("VIF DMA: DMA system shutdown complete");
}
