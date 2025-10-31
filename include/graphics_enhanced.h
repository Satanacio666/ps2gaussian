/*
 * SPLATSTORM X - Enhanced Graphics System API
 * Professional graphics system based on AthenaEnv integration
 */

#ifndef GRAPHICS_ENHANCED_H
#define GRAPHICS_ENHANCED_H

#include <gsKit.h>
#include <stdbool.h>

// Graphics statistics structure
typedef struct {
    bool initialized;
    int width;
    int height;
    int psm;
    int zbuffer_psm;
    bool double_buffering;
    bool zbuffer_enabled;
    float fps;
    bool vsync_enabled;
    size_t vram_used;
    size_t vram_total;
} graphics_stats_t;

// Core graphics functions
int splatstorm_init_graphics(void);
void splatstorm_shutdown_graphics(void);
void splatstorm_flip_screen(void);
void splatstorm_clear_screen(u32 color);

// Graphics state access
GSGLOBAL* splatstorm_get_gs_global(void);
bool splatstorm_graphics_is_initialized(void);
float splatstorm_get_fps(void);

// VSync control
void splatstorm_set_vsync(bool enabled);
bool splatstorm_get_vsync(void);

// Screen information
void splatstorm_get_screen_size(int* width, int* height);

// VRAM management
void* splatstorm_alloc_vram(size_t size);
void splatstorm_free_vram(void* ptr);

// Texture management
GSTEXTURE* splatstorm_create_texture(int width, int height, int psm);
void splatstorm_free_texture(GSTEXTURE* texture);
int splatstorm_upload_texture(GSTEXTURE* texture);

// Drawing primitives
void splatstorm_draw_pixel(int x, int y, u32 color);
void splatstorm_draw_line(int x1, int y1, int x2, int y2, u32 color);
void splatstorm_draw_rect(int x, int y, int width, int height, u32 color);

// Statistics
void splatstorm_get_graphics_stats(graphics_stats_t* stats);

#endif // GRAPHICS_ENHANCED_H
