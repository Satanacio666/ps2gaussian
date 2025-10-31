/*
 * SPLATSTORM X - Professional DMA System API
 * VIF packet management based on AthenaEnv implementation
 */

#ifndef VIF_DMA_H
#define VIF_DMA_H

// Include types header to avoid circular dependencies
#include "splatstorm_types.h"

#include "vif_macros_extended.h"

// VIF packet management
void splatstorm_vif_send_packet(void* packet, u32 vif_channel);
void* splatstorm_vif_create_packet(u32 size);
void splatstorm_vif_destroy_packet(void* packet);

// VU microcode upload
void splatstorm_vu1_upload_microcode(u32* start, u32* end);
void splatstorm_vu0_upload_microcode(u32* start, u32* end);

// VU1 control
void splatstorm_vu1_set_double_buffer_settings(u32 base, u32 offset);
void splatstorm_vu1_send_data(void* data, u32 size, u32 dest_addr);
void splatstorm_vu1_start_program(u32 start_addr);
void splatstorm_vu1_wait_finish(void);

// DMA chain management
void splatstorm_dma_send_chain(void* data, u32 size);
void splatstorm_dma_build_display_list(splat_t* splats, u32 count);

// DMA system control
int splatstorm_dma_init(void);
void splatstorm_dma_shutdown(void);

#endif // VIF_DMA_H
