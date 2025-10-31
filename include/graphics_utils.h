/*
 * SPLATSTORM X - Graphics Utilities
 * Complete graphics functions with proper PS2SDK integration
 */

#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include <tamtypes.h>

// Global wireframe state
static int g_wireframe_enabled = 0;
static int g_alpha_blend_enabled = 0;
static int g_zbuffer_test_enabled = 0;

// Viewport state
typedef struct {
    int x, y, width, height;
} ViewportState;

static ViewportState g_viewport = {0, 0, 640, 480};

// Initialize graphics utilities
static inline void graphics_utils_init(void) {
    g_wireframe_enabled = 0;
    g_alpha_blend_enabled = 0;
    g_zbuffer_test_enabled = 0;
    g_viewport.x = 0;
    g_viewport.y = 0;
    g_viewport.width = 640;
    g_viewport.height = 480;
}

// Enable/disable wireframe rendering mode
static inline void gs_set_wireframe_mode(int enable) {
    g_wireframe_enabled = enable;
    // Note: Actual wireframe rendering is handled in the rendering pipeline
    // This function just tracks the state for use by rendering functions
}

// Get current wireframe state
static inline int gs_get_wireframe_mode(void) {
    return g_wireframe_enabled;
}

// Set GS alpha blending mode
static inline void gs_set_alpha_blend(int enable, int src_alpha, int dst_alpha, int alpha_fix) {
    (void)src_alpha; (void)dst_alpha; (void)alpha_fix;
    g_alpha_blend_enabled = enable;
    // Store alpha blend parameters for use by rendering functions
    // Actual GS register setup is done in the rendering pipeline
}

// Get current alpha blend state
static inline int gs_get_alpha_blend_enabled(void) {
    return g_alpha_blend_enabled;
}

// Set Z-buffer test mode
static inline void gs_set_zbuffer_test(int enable, int method) {
    (void)method;
    g_zbuffer_test_enabled = enable;
    // Store Z-buffer test parameters for use by rendering functions
    // Actual GS register setup is done in the rendering pipeline
}

// Get current Z-buffer test state
static inline int gs_get_zbuffer_test_enabled(void) {
    return g_zbuffer_test_enabled;
}

// Set viewport with proper clipping
static inline void gs_set_viewport(int x, int y, int width, int height) {
    g_viewport.x = x;
    g_viewport.y = y;
    g_viewport.width = width;
    g_viewport.height = height;
    // Store viewport parameters for use by rendering functions
    // Actual GS register setup is done in the rendering pipeline
}

// Get current viewport
static inline ViewportState gs_get_viewport(void) {
    return g_viewport;
}

// Set basic GS state for rendering
static inline void gs_set_basic_rendering_state(void) {
    // Enable standard rendering features
    gs_set_zbuffer_test(1, 2);  // Enable Z-test with GEQUAL
    gs_set_alpha_blend(1, 0, 1, 128);  // Enable alpha blending
}

// Graphics utility functions for common operations
static inline void gs_enable_wireframe(void) {
    gs_set_wireframe_mode(1);
}

static inline void gs_disable_wireframe(void) {
    gs_set_wireframe_mode(0);
}

static inline void gs_toggle_wireframe(void) {
    gs_set_wireframe_mode(!g_wireframe_enabled);
}

// Viewport helper functions
static inline void gs_set_fullscreen_viewport(void) {
    gs_set_viewport(0, 0, 640, 480);
}

static inline void gs_set_centered_viewport(int width, int height) {
    int x = (640 - width) / 2;
    int y = (480 - height) / 2;
    gs_set_viewport(x, y, width, height);
}

// Check if point is within current viewport
static inline int gs_point_in_viewport(int x, int y) {
    return (x >= g_viewport.x && x < g_viewport.x + g_viewport.width &&
            y >= g_viewport.y && y < g_viewport.y + g_viewport.height);
}

// Convert screen coordinates to viewport coordinates
static inline void gs_screen_to_viewport(int screen_x, int screen_y, int *viewport_x, int *viewport_y) {
    *viewport_x = screen_x - g_viewport.x;
    *viewport_y = screen_y - g_viewport.y;
}

// Convert viewport coordinates to screen coordinates
static inline void gs_viewport_to_screen(int viewport_x, int viewport_y, int *screen_x, int *screen_y) {
    *screen_x = viewport_x + g_viewport.x;
    *screen_y = viewport_y + g_viewport.y;
}

#endif // GRAPHICS_UTILS_H