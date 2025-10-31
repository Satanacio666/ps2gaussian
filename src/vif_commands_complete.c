/*
 * SPLATSTORM X - Complete VIF Command Implementation
 * All 21 missing VIF command functions - NO STUBS OR PLACEHOLDERS
 * 
 * Based on vif_macros_extended.h requirements and PS2 VIF specification
 * Implements: UNPACK commands (4), CONTROL commands (5), SETUP commands (6), 
 *            TRANSFER commands (3), Packet management (3)
 */

#include "vif_macros_extended.h"
#include "splatstorm_x.h"
#include "gaussian_types.h"
#include <tamtypes.h>
#include <vif_codes.h>
#include <packet2.h>
#include <dma.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

// VIF command state tracking
static struct {
    bool initialized;
    u32 current_cycle;
    u32 current_offset;
    u32 current_base;
    u32 current_itop;
    u32 current_mode;
    u32 packets_created;
    u32 commands_sent;
    u64 total_data_transferred;
} g_vif_state = {0};

// VIF packet buffer management - COMPLETE IMPLEMENTATION
static struct {
    void* packet_buffer;
    u32 buffer_size;
    u32 buffer_used;
    u32 max_packet_size;
    bool buffer_allocated;
    // Current packet tracking for finalization
    void* current_packet;
    u32 current_used;
    u32 current_packet_size;
    bool packet_in_progress;
} g_vif_buffer = {0};

// Forward declarations
static void vif_ensure_initialized(void);
static u32* vif_get_packet_ptr(u32 size_needed);
static void vif_finalize_current_packet(void);
static int vif_send_packet_to_vu(void* packet, u32 size, int vu_unit);

/*
 * VIF UNPACK COMMANDS - COMPLETE IMPLEMENTATIONS
 */

// VIF UNPACK V4-32 - COMPLETE IMPLEMENTATION
void vif_unpack_v4_32(u16 addr, u8 num, u8 flags, void* data) {
    vif_ensure_initialized();
    
    if (!data || num == 0) {
        printf("VIF ERROR: Invalid parameters for vif_unpack_v4_32\n");
        return;
    }
    
    // Calculate packet size: VIF tag + data
    u32 data_size = num * 16; // 4 floats * 4 bytes each
    u32 packet_size = 16 + data_size; // VIF tag (16 bytes) + data
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for V4-32 unpack\n");
        return;
    }
    
    // Build VIF UNPACK tag
    u32 vif_tag = VIF_SET_UNPACK(addr, num, VIF_V4_32, flags);
    
    // Write VIF tag (as quadword)
    packet[0] = vif_tag;
    packet[1] = 0; // Padding
    packet[2] = 0; // Padding  
    packet[3] = 0; // Padding
    
    // Copy data
    memcpy(&packet[4], data, data_size);
    
    // Send to VU1
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: V4-32 UNPACK sent - addr=0x%04X, num=%u, size=%u bytes\n", 
           addr, num, data_size);
}

// VIF UNPACK V3-32 - COMPLETE IMPLEMENTATION  
void vif_unpack_v3_32(u16 addr, u8 num, u8 flags, void* data) {
    vif_ensure_initialized();
    
    if (!data || num == 0) {
        printf("VIF ERROR: Invalid parameters for vif_unpack_v3_32\n");
        return;
    }
    
    // Calculate packet size: VIF tag + data
    u32 data_size = num * 12; // 3 floats * 4 bytes each
    u32 packet_size = 16 + ((data_size + 15) & ~15); // Align to 16 bytes
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for V3-32 unpack\n");
        return;
    }
    
    // Build VIF UNPACK tag
    u32 vif_tag = VIF_SET_UNPACK(addr, num, VIF_V3_32, flags);
    
    // Write VIF tag
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy data with padding to 16-byte alignment
    u8* dest = (u8*)&packet[4];
    u8* src = (u8*)data;
    for (u32 i = 0; i < num; i++) {
        memcpy(dest, src, 12);
        dest += 16; // Pad to 16 bytes
        src += 12;
    }
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: V3-32 UNPACK sent - addr=0x%04X, num=%u, size=%u bytes\n", 
           addr, num, data_size);
}

// VIF UNPACK V2-32 - COMPLETE IMPLEMENTATION
void vif_unpack_v2_32(u16 addr, u8 num, u8 flags, void* data) {
    vif_ensure_initialized();
    
    if (!data || num == 0) {
        printf("VIF ERROR: Invalid parameters for vif_unpack_v2_32\n");
        return;
    }
    
    u32 data_size = num * 8; // 2 floats * 4 bytes each
    u32 packet_size = 16 + ((data_size + 15) & ~15);
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for V2-32 unpack\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_UNPACK(addr, num, VIF_V2_32, flags);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy data with padding
    u8* dest = (u8*)&packet[4];
    u8* src = (u8*)data;
    for (u32 i = 0; i < num; i++) {
        memcpy(dest, src, 8);
        dest += 16; // Pad to 16 bytes
        src += 8;
    }
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: V2-32 UNPACK sent - addr=0x%04X, num=%u, size=%u bytes\n", 
           addr, num, data_size);
}

// VIF UNPACK S-32 - COMPLETE IMPLEMENTATION
void vif_unpack_s_32(u16 addr, u8 num, u8 flags, void* data) {
    vif_ensure_initialized();
    
    if (!data || num == 0) {
        printf("VIF ERROR: Invalid parameters for vif_unpack_s_32\n");
        return;
    }
    
    u32 data_size = num * 4; // 1 float * 4 bytes each
    u32 packet_size = 16 + ((data_size + 15) & ~15);
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for S-32 unpack\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_UNPACK(addr, num, VIF_V1_32, flags);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy data with padding
    u8* dest = (u8*)&packet[4];
    u8* src = (u8*)data;
    for (u32 i = 0; i < num; i++) {
        memcpy(dest, src, 4);
        dest += 16; // Pad to 16 bytes
        src += 4;
    }
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: S-32 UNPACK sent - addr=0x%04X, num=%u, size=%u bytes\n", 
           addr, num, data_size);
}

/*
 * VIF CONTROL COMMANDS - COMPLETE IMPLEMENTATIONS
 */

// VIF MSCAL - COMPLETE IMPLEMENTATION
void vif_mscal(u16 addr) {
    vif_ensure_initialized();
    
    u32 packet_size = 16; // Single VIF command
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for MSCAL\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_MSCAL(addr);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: MSCAL sent - addr=0x%04X\n", addr);
}

// VIF MSCALF - COMPLETE IMPLEMENTATION
void vif_mscalf(u16 addr) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for MSCALF\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_MSCALF(addr);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: MSCALF sent - addr=0x%04X\n", addr);
}

// VIF FLUSH - COMPLETE IMPLEMENTATION
void vif_flush(void) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for FLUSH\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_FLUSH();
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: FLUSH sent\n");
}

// VIF FLUSHE - COMPLETE IMPLEMENTATION
void vif_flushe(void) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for FLUSHE\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_FLUSHE();
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: FLUSHE sent\n");
}

// VIF FLUSHA - COMPLETE IMPLEMENTATION
void vif_flusha(void) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for FLUSHA\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_FLUSHA();
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: FLUSHA sent\n");
}

/*
 * VIF SETUP COMMANDS - COMPLETE IMPLEMENTATIONS
 */

// VIF STCYCL - COMPLETE IMPLEMENTATION
void vif_stcycl(u8 cl, u8 wl) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for STCYCL\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_STCYCL(cl, wl);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.current_cycle = (wl << 8) | cl;
    g_vif_state.commands_sent++;
    
    printf("VIF: STCYCL sent - cl=%u, wl=%u\n", cl, wl);
}

// VIF OFFSET - COMPLETE IMPLEMENTATION
void vif_offset(u16 offset) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for OFFSET\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_OFFSET(offset);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.current_offset = offset;
    g_vif_state.commands_sent++;
    
    printf("VIF: OFFSET sent - offset=0x%04X\n", offset);
}

// VIF BASE - COMPLETE IMPLEMENTATION
void vif_base(u16 base) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for BASE\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_BASE(base);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.current_base = base;
    g_vif_state.commands_sent++;
    
    printf("VIF: BASE sent - base=0x%04X\n", base);
}

// VIF ITOP - COMPLETE IMPLEMENTATION
void vif_itop(u16 addr) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for ITOP\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_ITOP(addr);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.current_itop = addr;
    g_vif_state.commands_sent++;
    
    printf("VIF: ITOP sent - addr=0x%04X\n", addr);
}

// VIF STMOD - COMPLETE IMPLEMENTATION
void vif_stmod(u8 mode) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for STMOD\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_STMOD(mode);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.current_mode = mode;
    g_vif_state.commands_sent++;
    
    printf("VIF: STMOD sent - mode=%u\n", mode);
}

// VIF MARK - COMPLETE IMPLEMENTATION
void vif_mark(u16 mark) {
    vif_ensure_initialized();
    
    u32 packet_size = 16;
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for MARK\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_MARK(mark);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    
    printf("VIF: MARK sent - mark=0x%04X\n", mark);
}

/*
 * VIF TRANSFER COMMANDS - COMPLETE IMPLEMENTATIONS
 */

// VIF DIRECT - COMPLETE IMPLEMENTATION
void vif_direct(u16 size, void* data) {
    vif_ensure_initialized();
    
    if (!data || size == 0) {
        printf("VIF ERROR: Invalid parameters for vif_direct\n");
        return;
    }
    
    u32 data_size = size * 16; // Size is in quadwords
    u32 packet_size = 16 + data_size;
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for DIRECT\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_DIRECT(size);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy GIF data
    memcpy(&packet[4], data, data_size);
    
    // Send directly to GS via GIF
    vif_send_packet_to_vu(packet, packet_size, 0); // 0 = GIF path
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: DIRECT sent - size=%u qwords, data_size=%u bytes\n", size, data_size);
}

// VIF DIRECTHL - COMPLETE IMPLEMENTATION
void vif_directhl(u16 addr, void* data, u16 size) {
    vif_ensure_initialized();
    
    if (!data || size == 0) {
        printf("VIF ERROR: Invalid parameters for vif_directhl\n");
        return;
    }
    
    u32 data_size = size * 16; // Size is in quadwords
    u32 packet_size = 16 + data_size;
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for DIRECTHL\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_DIRECTHL(addr);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy GIF data
    memcpy(&packet[4], data, data_size);
    
    // Send to GS via GIF with HL flag
    vif_send_packet_to_vu(packet, packet_size, 0); // 0 = GIF path
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: DIRECTHL sent - addr=0x%04X, size=%u qwords\n", addr, size);
}

// VIF MPG - COMPLETE IMPLEMENTATION
void vif_mpg(u16 loadaddr, u16 size, void* microcode) {
    vif_ensure_initialized();
    
    if (!microcode || size == 0) {
        printf("VIF ERROR: Invalid parameters for vif_mpg\n");
        return;
    }
    
    u32 data_size = size * 8; // Size is in double-words (64-bit)
    u32 packet_size = 16 + ((data_size + 15) & ~15); // Align to 16 bytes
    
    u32* packet = vif_get_packet_ptr(packet_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate packet buffer for MPG\n");
        return;
    }
    
    u32 vif_tag = VIF_SET_MPG(loadaddr, size);
    
    packet[0] = vif_tag;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0;
    
    // Copy microcode data
    memcpy(&packet[4], microcode, data_size);
    
    // Send to VU1 microcode memory
    vif_send_packet_to_vu(packet, packet_size, 1);
    
    g_vif_state.commands_sent++;
    g_vif_state.total_data_transferred += data_size;
    
    printf("VIF: MPG sent - loadaddr=0x%04X, size=%u dwords, data_size=%u bytes\n", 
           loadaddr, size, data_size);
}

/*
 * VIF PACKET MANAGEMENT - COMPLETE IMPLEMENTATIONS
 */

// Create optimized packet - COMPLETE IMPLEMENTATION
void* vif_create_packet_optimized(u32 max_size) {
    vif_ensure_initialized();
    
    if (max_size == 0) {
        max_size = 64 * 1024; // Default 64KB packet
    }
    
    // Align to 128 bytes for DMA efficiency
    max_size = (max_size + 127) & ~127;
    
    void* packet = memalign(128, max_size);
    if (!packet) {
        printf("VIF ERROR: Failed to allocate optimized packet buffer (%u bytes)\n", max_size);
        return NULL;
    }
    
    memset(packet, 0, max_size);
    
    g_vif_state.packets_created++;
    
    printf("VIF: Optimized packet created - size=%u bytes, aligned to 128 bytes\n", max_size);
    
    return packet;
}

// Finalize packet - COMPLETE IMPLEMENTATION
void vif_finalize_packet(void* packet, u32 used_size) {
    if (!packet) {
        printf("VIF ERROR: Cannot finalize NULL packet\n");
        return;
    }
    
    // Add END tag if needed
    u32* end_ptr = (u32*)((u8*)packet + used_size);
    if (used_size % 16 == 0) {
        *end_ptr = VIF_SET_END();
        used_size += 16;
    }
    
    // Ensure packet is properly aligned
    u32 aligned_size = (used_size + 15) & ~15;
    if (aligned_size > used_size) {
        memset((u8*)packet + used_size, 0, aligned_size - used_size);
    }
    
    printf("VIF: Packet finalized - used_size=%u, aligned_size=%u\n", used_size, aligned_size);
}

// Get packet size - COMPLETE IMPLEMENTATION
u32 vif_get_packet_size(void* packet) {
    if (!packet) {
        return 0;
    }
    
    // Scan packet to find actual size
    u32* ptr = (u32*)packet;
    u32 size = 0;
    
    while (size < 64 * 1024) { // Maximum reasonable packet size
        u32 cmd = ptr[0];
        
        // Check for END command
        if ((cmd & 0x7F000000) == 0x00000000) { // NOP/END
            size += 16;
            break;
        }
        
        // Decode command to get data size
        u8 cmd_type = (cmd >> 24) & 0x7F;
        u32 cmd_size = 16; // Base command size
        
        if (cmd_type >= 0x60 && cmd_type <= 0x6F) { // UNPACK
            u8 num = cmd & 0xFF;
            u8 vn = (cmd_type >> 2) & 0x3;
            u8 vl = cmd_type & 0x3;
            
            // Calculate data size based on format
            u32 element_size = (1 << vl) * (vn + 1);
            cmd_size += num * element_size;
            cmd_size = (cmd_size + 15) & ~15; // Align to 16 bytes
        } else if (cmd_type == 0x50) { // DIRECT
            u16 direct_size = cmd & 0xFFFF;
            cmd_size += direct_size * 16;
        } else if (cmd_type == 0x4A) { // MPG
            u16 mpg_size = cmd & 0xFFFF;
            cmd_size += mpg_size * 8;
            cmd_size = (cmd_size + 15) & ~15;
        }
        
        size += cmd_size;
        ptr = (u32*)((u8*)packet + size);
    }
    
    return size;
}

/*
 * INTERNAL HELPER FUNCTIONS - COMPLETE IMPLEMENTATIONS
 */

// Ensure VIF system is initialized
static void vif_ensure_initialized(void) {
    if (g_vif_state.initialized) {
        return;
    }
    
    // Initialize VIF state
    memset(&g_vif_state, 0, sizeof(g_vif_state));
    g_vif_state.current_cycle = 0x0101; // Default CL=1, WL=1
    g_vif_state.current_offset = 0;
    g_vif_state.current_base = 0;
    g_vif_state.current_itop = 0;
    g_vif_state.current_mode = 0;
    g_vif_state.packets_created = 0;
    g_vif_state.commands_sent = 0;
    g_vif_state.total_data_transferred = 0;
    g_vif_state.initialized = true;
    
    // Initialize packet buffer
    g_vif_buffer.buffer_size = 1024 * 1024; // 1MB buffer
    g_vif_buffer.max_packet_size = 64 * 1024; // 64KB max packet
    g_vif_buffer.packet_buffer = memalign(128, g_vif_buffer.buffer_size);
    if (g_vif_buffer.packet_buffer) {
        memset(g_vif_buffer.packet_buffer, 0, g_vif_buffer.buffer_size);
        g_vif_buffer.buffer_allocated = true;
        g_vif_buffer.buffer_used = 0;
    } else {
        printf("VIF ERROR: Failed to allocate packet buffer\n");
        g_vif_buffer.buffer_allocated = false;
    }
    
    printf("VIF: System initialized - buffer_size=%u bytes\n", g_vif_buffer.buffer_size);
}

// Get packet pointer with size check
static u32* vif_get_packet_ptr(u32 size_needed) {
    if (!g_vif_buffer.buffer_allocated) {
        return NULL;
    }
    
    // Align size to 16 bytes
    size_needed = (size_needed + 15) & ~15;
    
    // Check if we have enough space
    if (g_vif_buffer.buffer_used + size_needed > g_vif_buffer.buffer_size) {
        // Reset buffer (simple circular buffer)
        g_vif_buffer.buffer_used = 0;
        printf("VIF: Buffer reset due to overflow\n");
    }
    
    u32* ptr = (u32*)((u8*)g_vif_buffer.packet_buffer + g_vif_buffer.buffer_used);
    g_vif_buffer.buffer_used += size_needed;
    
    return ptr;
}

// Send packet to VU via DMA
static int vif_send_packet_to_vu(void* packet, u32 size, int vu_unit) {
    if (!packet || size == 0) {
        return -1;
    }
    
    // Align size to quadwords
    u32 qwc = (size + 15) / 16;
    
    // Select DMA channel based on VU unit
    int channel = (vu_unit == 1) ? DMA_CHANNEL_VIF1 : DMA_CHANNEL_GIF;
    
    // Send via DMA
    int result = dma_channel_send_normal(channel, packet, qwc, 0, 0);
    if (result != 0) {
        printf("VIF ERROR: DMA send failed - channel=%d, result=%d\n", channel, result);
        return result;
    }
    
    // Wait for completion
    result = dma_channel_wait(channel, 1000); // 1 second timeout
    if (result != 0) {
        printf("VIF ERROR: DMA wait failed - channel=%d, result=%d\n", channel, result);
        return result;
    }
    
    return 0;
}

/*
 * VIF SYSTEM STATUS AND DEBUGGING - COMPLETE IMPLEMENTATIONS
 */

// Get VIF statistics
void vif_get_stats(u32* packets_created, u32* commands_sent, u64* bytes_transferred) {
    if (packets_created) *packets_created = g_vif_state.packets_created;
    if (commands_sent) *commands_sent = g_vif_state.commands_sent;
    if (bytes_transferred) *bytes_transferred = g_vif_state.total_data_transferred;
}

// Print VIF status
void vif_print_status(void) {
    printf("VIF System Status:\n");
    printf("  Initialized: %s\n", g_vif_state.initialized ? "Yes" : "No");
    printf("  Packets created: %u\n", g_vif_state.packets_created);
    printf("  Commands sent: %u\n", g_vif_state.commands_sent);
    printf("  Data transferred: %llu bytes\n", g_vif_state.total_data_transferred);
    printf("  Current state:\n");
    printf("    CYCLE: CL=%u, WL=%u\n", 
           g_vif_state.current_cycle & 0xFF, 
           (g_vif_state.current_cycle >> 8) & 0xFF);
    printf("    OFFSET: 0x%04X\n", g_vif_state.current_offset);
    printf("    BASE: 0x%04X\n", g_vif_state.current_base);
    printf("    ITOP: 0x%04X\n", g_vif_state.current_itop);
    printf("    MODE: %u\n", g_vif_state.current_mode);
    printf("  Buffer status:\n");
    printf("    Allocated: %s\n", g_vif_buffer.buffer_allocated ? "Yes" : "No");
    printf("    Size: %u bytes\n", g_vif_buffer.buffer_size);
    printf("    Used: %u bytes (%.1f%%)\n", 
           g_vif_buffer.buffer_used,
           (float)g_vif_buffer.buffer_used / g_vif_buffer.buffer_size * 100.0f);
}

// Cleanup VIF system
void vif_cleanup(void) {
    if (g_vif_buffer.buffer_allocated && g_vif_buffer.packet_buffer) {
        free(g_vif_buffer.packet_buffer);
        g_vif_buffer.packet_buffer = NULL;
        g_vif_buffer.buffer_allocated = false;
    }
    
    memset(&g_vif_state, 0, sizeof(g_vif_state));
    memset(&g_vif_buffer, 0, sizeof(g_vif_buffer));
    
    printf("VIF: System cleaned up\n");
}

// Finalize current packet - COMPLETE IMPLEMENTATION
static void vif_finalize_current_packet(void) {
    if (!g_vif_buffer.current_packet) {
        return; // No current packet to finalize
    }
    
    // Align the used size to 16-byte boundary
    u32 aligned_size = (g_vif_buffer.current_used + 15) & ~15;
    
    // Update packet header with final size
    packet2_t* packet = (packet2_t*)g_vif_buffer.current_packet;
    if (packet && packet->base) {
        // Set the final QWC (quadword count)
        u32 qwc = aligned_size / 16;
        // Update DMA tag if present
        if (qwc > 0) {
            u64* dma_tag = (u64*)packet->base;
            *dma_tag = (*dma_tag & 0xFFFF0000FFFFFFFFULL) | ((u64)qwc << 32);
        }
    }
    
    // Mark packet as finalized
    g_vif_buffer.current_packet = NULL;
    g_vif_buffer.current_used = 0;
    
    printf("VIF: Current packet finalized - size=%u bytes\n", aligned_size);
}