/*
 * SPLATSTORM X - Optimized DMA Header
 * Declarations for optimized DMA packet construction
 */

#ifndef DMA_OPTIMIZED_H
#define DMA_OPTIMIZED_H

#include <tamtypes.h>

// Forward declarations - PackedSplat defined in memory_optimized.h

// DMA packet statistics structure
typedef struct {
    u32 packets_sent;
    u32 total_splats;
    u32 batch_count;
    u32 avg_batch_size;
} DMAPacketStats;

// Function declarations
void build_and_send_packet_optimized(PackedSplat* splatArray, int count, float* mvp_matrix);
void process_splats_batched(PackedSplat* splats, int total_count, float* mvp_matrix);
void build_gif_packet_optimized(PackedSplat* splats, int count);
void vu_wait_for_completion(void);
void get_dma_packet_stats(DMAPacketStats* stats);

#endif // DMA_OPTIMIZED_H