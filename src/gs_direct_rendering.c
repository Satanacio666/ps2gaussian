/*
 * SPLATSTORM X - Direct GS Register Rendering
 * Bypass gsKit overhead for maximum fillrate performance
 * Direct register writes optimized for Gaussian splat rendering
 */

#include "splatstorm_x.h"
#include "gaussian_types.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations
void gs_set_primitive_type(int type);
void gs_setup_rendering_context(void);

// GS register addresses (direct hardware access)
#define GS_PMODE    ((volatile u64*)0x12000000)
#define GS_SMODE2   ((volatile u64*)0x12000020)
#define GS_DISPFB1  ((volatile u64*)0x12000070)
#define GS_DISPLAY1 ((volatile u64*)0x12000080)
#define GS_DISPFB2  ((volatile u64*)0x12000090)
#define GS_DISPLAY2 ((volatile u64*)0x120000A0)
#define GS_EXTBUF   ((volatile u64*)0x120000B0)
#define GS_EXTDATA  ((volatile u64*)0x120000C0)
#define GS_EXTWRITE ((volatile u64*)0x120000D0)
#define GS_BGCOLOR  ((volatile u64*)0x120000E0)

// GS drawing registers
#define GS_PRIM     ((volatile u64*)0x12000000)
#define GS_RGBAQ    ((volatile u64*)0x12000010)
#define GS_ST       ((volatile u64*)0x12000020)
#define GS_UV       ((volatile u64*)0x12000030)
#define GS_XYZ2     ((volatile u64*)0x12000040)
#define GS_XYZ3     ((volatile u64*)0x12000050)
#define GS_TEX0_1   ((volatile u64*)0x12000060)
#define GS_TEX0_2   ((volatile u64*)0x12000070)
#define GS_CLAMP_1  ((volatile u64*)0x12000080)
#define GS_CLAMP_2  ((volatile u64*)0x12000090)
#define GS_FOG      ((volatile u64*)0x120000A0)
#define GS_XYZF2    ((volatile u64*)0x120000B0)
#define GS_XYZF3    ((volatile u64*)0x120000C0)
#define GS_AD       ((volatile u64*)0x120000D0)
#define GS_NOP      ((volatile u64*)0x120000F0)

// GS context registers
#define GS_FRAME_1  ((volatile u64*)0x12000040)
#define GS_FRAME_2  ((volatile u64*)0x12000050)
#define GS_ZBUF_1   ((volatile u64*)0x12000060)
#define GS_ZBUF_2   ((volatile u64*)0x12000070)
#define GS_XYOFFSET_1 ((volatile u64*)0x12000080)
#define GS_XYOFFSET_2 ((volatile u64*)0x12000090)
#define GS_PRMODECONT ((volatile u64*)0x120000A0)
#define GS_PRMODE   ((volatile u64*)0x120000B0)
#define GS_TEXCLUT  ((volatile u64*)0x120000C0)
#define GS_SCANMSK1 ((volatile u64*)0x120000D0)
#define GS_SCANMSK2 ((volatile u64*)0x120000E0)
#define GS_MIPTBP1_1 ((volatile u64*)0x120000F0)
#define GS_MIPTBP1_2 ((volatile u64*)0x12000100)
#define GS_MIPTBP2_1 ((volatile u64*)0x12000110)
#define GS_MIPTBP2_2 ((volatile u64*)0x12000120)
#define GS_TEXA     ((volatile u64*)0x12000130)
#define GS_FOGCOL   ((volatile u64*)0x12000140)
#define GS_TEXFLUSH ((volatile u64*)0x12000150)
#define GS_SCISSOR_1 ((volatile u64*)0x12000160)
#define GS_SCISSOR_2 ((volatile u64*)0x12000170)
#define GS_ALPHA_1  ((volatile u64*)0x12000180)
#define GS_ALPHA_2  ((volatile u64*)0x12000190)
#define GS_DIMX     ((volatile u64*)0x120001A0)
#define GS_DTHE     ((volatile u64*)0x120001B0)
#define GS_COLCLAMP ((volatile u64*)0x120001C0)
#define GS_TEST_1   ((volatile u64*)0x120001D0)
#define GS_TEST_2   ((volatile u64*)0x120001E0)
#define GS_PABE     ((volatile u64*)0x120001F0)
#define GS_FBA_1    ((volatile u64*)0x12000200)
#define GS_FBA_2    ((volatile u64*)0x12000210)

// GS register value construction macros
#define GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
    ((u64)(prim) | ((u64)(iip) << 3) | ((u64)(tme) << 4) | \
     ((u64)(fge) << 5) | ((u64)(abe) << 6) | ((u64)(aa1) << 7) | \
     ((u64)(fst) << 8) | ((u64)(ctxt) << 9) | ((u64)(fix) << 10))

#define GS_SET_RGBAQ(r, g, b, a, q) \
    ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | \
     ((u64)(a) << 24) | ((u64)(q) << 32))

#define GS_SET_XYZ(x, y, z) \
    ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))

#define GS_SET_ST(s, t) \
    ((u64)(s) | ((u64)(t) << 32))

#define GS_SET_TEX0(tbp0, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((u64)(tbp0) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | \
     ((u64)(tw) << 26) | ((u64)(th) << 30) | ((u64)(tcc) << 34) | \
     ((u64)(tfx) << 35) | ((u64)(cbp) << 37) | ((u64)(cpsm) << 51) | \
     ((u64)(csm) << 55) | ((u64)(csa) << 56) | ((u64)(cld) << 61))

#define GS_SET_ALPHA(a, b, c, d, fix) \
    ((u64)(a) | ((u64)(b) << 2) | ((u64)(c) << 4) | \
     ((u64)(d) << 6) | ((u64)(fix) << 32))

#define GS_SET_TEST(ate, atst, aref, afail, date, datst, zte, ztst) \
    ((u64)(ate) | ((u64)(atst) << 1) | ((u64)(aref) << 4) | \
     ((u64)(afail) << 12) | ((u64)(date) << 14) | \
     ((u64)(datst) << 15) | ((u64)(zte) << 17) | ((u64)(ztst) << 18))

#define GS_SET_ZBUF(zbp, psm, zmsk) \
    ((u64)(zbp) | ((u64)(psm) << 24) | ((u64)(zmsk) << 32))

#define GS_SET_FRAME(fbp, fbw, psm, fbmsk) \
    ((u64)(fbp) | ((u64)(fbw) << 16) | ((u64)(psm) << 24) | ((u64)(fbmsk) << 32))

// Primitive types
#define GS_PRIM_POINT       0
#define GS_PRIM_LINE        1
#define GS_PRIM_LINESTRIP   2
#define GS_PRIM_TRIANGLE    3
#define GS_PRIM_TRISTRIP    4
#define GS_PRIM_TRIFAN      5
#define GS_PRIM_SPRITE      6

// Pixel storage modes
#define GS_PSM_CT32         0
#define GS_PSM_CT24         1
#define GS_PSM_CT16         2
#define GS_PSM_CT16S        10
#define GS_PSMZ_32          0
#define GS_PSMZ_24          1
#define GS_PSMZ_16          2
#define GS_PSMZ_16S         10

// Alpha blending modes
#define GS_BLEND_SRC_ALPHA      0
#define GS_BLEND_DST_ALPHA      1
#define GS_BLEND_SRC_ALPHA_INV  2
#define GS_BLEND_DST_ALPHA_INV  3

// Z-test modes
#define GS_ZTEST_NEVER      0
#define GS_ZTEST_ALWAYS     1
#define GS_ZTEST_GEQUAL     2
#define GS_ZTEST_GREATER    3

// Direct rendering context
typedef struct {
    u32 frame_buffer_base;      // Frame buffer base address
    u32 z_buffer_base;          // Z-buffer base address
    u32 texture_base;           // Texture base address
    u16 screen_width;           // Screen width
    u16 screen_height;          // Screen height
    u8 current_context;         // Current drawing context (0 or 1)
    bool texturing_enabled;     // Texture mapping enabled
    bool alpha_blending_enabled; // Alpha blending enabled
    bool z_testing_enabled;     // Z-testing enabled
} GSDirectContext;

static GSDirectContext gs_context;

// Initialize direct GS rendering for Gaussian splats
int gs_direct_init(u16 width, u16 height, u32 frame_base, u32 z_base) {
    gs_context.screen_width = width;
    gs_context.screen_height = height;
    gs_context.frame_buffer_base = frame_base;
    gs_context.z_buffer_base = z_base;
    gs_context.texture_base = 0x300;  // Start textures at page 0x300
    gs_context.current_context = 0;
    gs_context.texturing_enabled = true;
    gs_context.alpha_blending_enabled = true;
    gs_context.z_testing_enabled = true;
    
    // Set up frame buffer
    *GS_FRAME_1 = GS_SET_FRAME(frame_base, width / 64, GS_PSM_CT32, 0);
    
    // Set up Z-buffer
    *GS_ZBUF_1 = GS_SET_ZBUF(z_base, GS_PSMZ_24, 0);
    
    // Set up alpha blending for Gaussian splatting
    // Cs*As + Cd*(1-As) for proper alpha compositing
    *GS_ALPHA_1 = GS_SET_ALPHA(GS_BLEND_SRC_ALPHA, GS_BLEND_DST_ALPHA_INV, 
                               GS_BLEND_SRC_ALPHA, GS_BLEND_DST_ALPHA_INV, 0x80);
    
    // Set up Z-testing (greater-equal for back-to-front rendering)
    *GS_TEST_1 = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_GEQUAL);
    
    // Enable per-pixel alpha blending
    *GS_PABE = 1;
    
    // Set up texture filtering and clamping
    *GS_CLAMP_1 = 0;  // No clamping (wrap mode)
    
    return 0;
}

// Set up texturing for LUT sampling
void gs_direct_setup_lut_texturing(void) {
    // Set up exponential LUT texture
    *GS_TEX0_1 = GS_SET_TEX0(0x100,    // TBP0: texture base page
                             4,        // TBW: texture buffer width (256/64 = 4)
                             GS_PSM_CT32, // PSM: 32-bit color
                             8,        // TW: texture width (log2(256) = 8)
                             0,        // TH: texture height (log2(1) = 0) - 1D texture
                             1,        // TCC: RGBA
                             0,        // TFX: modulate
                             0,        // CBP: CLUT base
                             0,        // CPSM: CLUT format
                             0,        // CSM: CLUT storage mode
                             0,        // CSA: CLUT entry offset
                             1);       // CLD: CLUT load control
    
    gs_context.texturing_enabled = true;
}

// Set up footprint atlas texturing
void gs_direct_setup_atlas_texturing(void) {
    // Set up footprint atlas texture (256x256)
    *GS_TEX0_1 = GS_SET_TEX0(0x400,    // TBP0: atlas base page
                             4,        // TBW: texture buffer width (256/64 = 4)
                             GS_PSM_CT32, // PSM: 32-bit color
                             8,        // TW: texture width (log2(256) = 8)
                             8,        // TH: texture height (log2(256) = 8)
                             1,        // TCC: RGBA
                             0,        // TFX: modulate
                             0,        // CBP: CLUT base
                             0,        // CPSM: CLUT format
                             0,        // CSM: CLUT storage mode
                             0,        // CSA: CLUT entry offset
                             1);       // CLD: CLUT load control
    
    gs_context.texturing_enabled = true;
}

// Clear screen with background color
void gs_direct_clear_screen(u8 r, u8 g, u8 b) {
    // Set primitive to sprite (for screen-filling quad)
    *GS_PRIM = GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 1, 0, 0);
    
    // Set color
    *GS_RGBAQ = GS_SET_RGBAQ(r, g, b, 0x80, 0);
    
    // Draw full-screen sprite
    *GS_XYZ2 = GS_SET_XYZ(0, 0, 0);
    *GS_XYZ2 = GS_SET_XYZ(gs_context.screen_width << 4, gs_context.screen_height << 4, 0);
}

// Render a single Gaussian splat as a textured quad
void gs_direct_render_splat_quad(const GaussianSplat2D* splat, 
                                float u_base, float v_base, 
                                float u_scale, float v_scale) {
    // Set primitive to triangle strip for quad
    *GS_PRIM = GS_SET_PRIM(GS_PRIM_TRISTRIP, 1, 1, 0, 1, 0, 0, 0, 0);
    
    // Convert fixed-point screen coordinates to GS format (4.4 subpixel)
    u16 center_x = (u16)(fixed_to_float(splat->screen_pos[0]) * 16.0f);
    u16 center_y = (u16)(fixed_to_float(splat->screen_pos[1]) * 16.0f);
    u16 radius = (u16)(fixed_to_float(splat->radius) * 16.0f);
    
    // Calculate quad vertices
    u16 x1 = center_x - radius;
    u16 y1 = center_y - radius;
    u16 x2 = center_x + radius;
    u16 y2 = center_y + radius;
    
    // Z-coordinate for depth testing (convert to 24-bit)
    u32 z = (u32)(fixed_to_float(splat->depth) * 0xFFFFFF);
    
    // Set color and alpha
    *GS_RGBAQ = GS_SET_RGBAQ(splat->color[0], splat->color[1], 
                            splat->color[2], splat->color[3], 0);
    
    // Vertex 0: top-left
    *GS_ST = GS_SET_ST((u32)(u_base * 0x10000), (u32)(v_base * 0x10000));
    *GS_XYZ2 = GS_SET_XYZ(x1, y1, z);
    
    // Vertex 1: top-right
    *GS_ST = GS_SET_ST((u32)((u_base + u_scale) * 0x10000), (u32)(v_base * 0x10000));
    *GS_XYZ2 = GS_SET_XYZ(x2, y1, z);
    
    // Vertex 2: bottom-left
    *GS_ST = GS_SET_ST((u32)(u_base * 0x10000), (u32)((v_base + v_scale) * 0x10000));
    *GS_XYZ2 = GS_SET_XYZ(x1, y2, z);
    
    // Vertex 3: bottom-right
    *GS_ST = GS_SET_ST((u32)((u_base + u_scale) * 0x10000), (u32)((v_base + v_scale) * 0x10000));
    *GS_XYZ2 = GS_SET_XYZ(x2, y2, z);
}

// Render a batch of Gaussian splats using direct GS access
void gs_direct_render_splat_batch(const GaussianSplat2D* splats, 
                                 const u16* indices, int count) {
    if (count == 0) return;
    
    // Set up for splat rendering
    gs_direct_setup_atlas_texturing();
    
    // Render each splat
    for (int i = 0; i < count; i++) {
        u16 splat_idx = indices[i];
        const GaussianSplat2D* splat = &splats[splat_idx];
        
        // Get atlas UV coordinates (simplified - would use eigenvalues/rotation)
        float u_base = 0.0f;   // Would be computed from splat properties
        float v_base = 0.0f;
        float u_scale = 0.125f; // 1/8 for 8x8 atlas
        float v_scale = 0.125f;
        
        // Render the quad
        gs_direct_render_splat_quad(splat, u_base, v_base, u_scale, v_scale);
    }
}

// Render all tiles with their splats
void gs_direct_render_tiles(const GaussianSplat2D* splats, 
                           const TileRange* tile_ranges,
                           const u16* sort_indices) {
    // Clear screen
    gs_direct_clear_screen(0, 0, 0);
    
    // Render tiles in order (back-to-front within each tile)
    for (int tile_id = 0; tile_id < MAX_TILES; tile_id++) {
        const TileRange* range = &tile_ranges[tile_id];
        if (range->count == 0) continue;
        
        // Get sorted indices for this tile
        const u16* tile_indices = &sort_indices[range->start_index];
        
        // Render all splats in this tile
        gs_direct_render_splat_batch(splats, tile_indices, range->count);
    }
}

// Optimized rendering for simple Gaussian falloff (no atlas)
void gs_direct_render_simple_splats(const GaussianSplat2D* splats, 
                                   const u16* indices, int count) {
    if (count == 0) return;
    
    // Disable texturing for simple rendering
    *GS_PRIM = GS_SET_PRIM(GS_PRIM_TRISTRIP, 1, 0, 0, 1, 0, 1, 0, 0);
    
    for (int i = 0; i < count; i++) {
        u16 splat_idx = indices[i];
        const GaussianSplat2D* splat = &splats[splat_idx];
        
        // Convert coordinates
        u16 center_x = (u16)(fixed_to_float(splat->screen_pos[0]) * 16.0f);
        u16 center_y = (u16)(fixed_to_float(splat->screen_pos[1]) * 16.0f);
        u16 radius = (u16)(fixed_to_float(splat->radius) * 16.0f);
        
        u16 x1 = center_x - radius;
        u16 y1 = center_y - radius;
        u16 x2 = center_x + radius;
        u16 y2 = center_y + radius;
        
        u32 z = (u32)(fixed_to_float(splat->depth) * 0xFFFFFF);
        
        // Set color with alpha falloff approximation
        u8 alpha = splat->color[3];
        *GS_RGBAQ = GS_SET_RGBAQ(splat->color[0], splat->color[1], 
                                splat->color[2], alpha, 0);
        
        // Render quad (triangle strip)
        *GS_XYZ2 = GS_SET_XYZ(x1, y1, z);
        *GS_XYZ2 = GS_SET_XYZ(x2, y1, z);
        *GS_XYZ2 = GS_SET_XYZ(x1, y2, z);
        *GS_XYZ2 = GS_SET_XYZ(x2, y2, z);
    }
}

// Swap frame buffers for double buffering
void gs_direct_swap_buffers(void) {
    // Toggle between contexts
    gs_context.current_context = 1 - gs_context.current_context;
    
    // Update display buffer
    if (gs_context.current_context == 0) {
        *GS_DISPFB1 = gs_context.frame_buffer_base;
    } else {
        *GS_DISPFB2 = gs_context.frame_buffer_base + 
                     (gs_context.screen_width * gs_context.screen_height * 4);
    }
}

// Get performance statistics
void gs_direct_get_stats(u32* triangles_rendered, u32* pixels_filled, 
                        u32* texture_samples, float* fillrate_mpixels_sec) {
    // These would be maintained by counting operations
    // For now, return placeholder values
    *triangles_rendered = 0;
    *pixels_filled = 0;
    *texture_samples = 0;
    *fillrate_mpixels_sec = 0.0f;
}

// Debug function to render wireframe mode
void gs_direct_render_wireframe(const GaussianSplat2D* splats, 
                               const u16* indices, int count) {
    // Set primitive to line strip
    *GS_PRIM = GS_SET_PRIM(GS_PRIM_LINESTRIP, 0, 0, 0, 0, 0, 1, 0, 0);
    
    for (int i = 0; i < count; i++) {
        u16 splat_idx = indices[i];
        const GaussianSplat2D* splat = &splats[splat_idx];
        
        u16 center_x = (u16)(fixed_to_float(splat->screen_pos[0]) * 16.0f);
        u16 center_y = (u16)(fixed_to_float(splat->screen_pos[1]) * 16.0f);
        u16 radius = (u16)(fixed_to_float(splat->radius) * 16.0f);
        
        u16 x1 = center_x - radius;
        u16 y1 = center_y - radius;
        u16 x2 = center_x + radius;
        u16 y2 = center_y + radius;
        
        // Set wireframe color
        *GS_RGBAQ = GS_SET_RGBAQ(255, 255, 255, 128, 0);
        
        // Draw quad outline
        *GS_XYZ2 = GS_SET_XYZ(x1, y1, 0);
        *GS_XYZ2 = GS_SET_XYZ(x2, y1, 0);
        *GS_XYZ2 = GS_SET_XYZ(x2, y2, 0);
        *GS_XYZ2 = GS_SET_XYZ(x1, y2, 0);
        *GS_XYZ2 = GS_SET_XYZ(x1, y1, 0);  // Close the loop
    }
}

/*
 * Flush the GS rendering pipeline
 */
void gs_flush_rendering_pipeline(void) {
    debug_log_verbose("Flushing GS rendering pipeline");
    
    // Send a NOP to ensure all commands are processed
    *GS_NOP = 0;
    
    // Wait for GS to finish processing
    // In a real implementation, we might check GS_CSR for completion
    volatile u32 dummy = *(volatile u32*)GS_CSR;
    (void)dummy;  // Prevent optimization
    
    debug_log_verbose("GS pipeline flushed");
}

/*
 * Direct render splats with optimal GS register usage
 */
void gs_direct_render_splats(GaussianSplat3D* splats, int count) {
    debug_log_info("Direct rendering %d splats", count);
    
    if (!splats || count <= 0) {
        debug_log_error("Invalid splat parameters");
        return;
    }
    
    // Set up rendering context for splats
    gs_setup_rendering_context();
    
    // Render each splat as a textured quad
    for (int i = 0; i < count; i++) {
        GaussianSplat3D* splat = &splats[i];
        
        // Convert 3D splat to 2D screen coordinates (simplified projection)
        float screen_x = (FIXED16_TO_FLOAT(splat->pos[0]) + 1.0f) * 320.0f;
        float screen_y = (FIXED16_TO_FLOAT(splat->pos[1]) + 1.0f) * 240.0f;
        float scale_x = 20.0f; // Use fixed scale for now
        float scale_y = 20.0f; // Use fixed scale for now
        
        // Calculate quad vertices
        u32 x1 = (u32)((screen_x - scale_x) * 16.0f);  // Convert to GS coordinates
        u32 y1 = (u32)((screen_y - scale_y) * 16.0f);
        u32 x2 = (u32)((screen_x + scale_x) * 16.0f);
        u32 y2 = (u32)((screen_y + scale_y) * 16.0f);
        
        // Set color with opacity
        u8 r = (u8)(splat->color[0] * 255.0f);
        u8 g = (u8)(splat->color[1] * 255.0f);
        u8 b = (u8)(splat->color[2] * 255.0f);
        u8 a = (u8)(splat->opacity * 128.0f);
        
        *GS_RGBAQ = GS_SET_RGBAQ(r, g, b, a, 0);
        
        // Render quad using triangle strip
        *GS_XYZ2 = GS_SET_XYZ(x1, y1, 0);
        *GS_XYZ2 = GS_SET_XYZ(x2, y1, 0);
        *GS_XYZ2 = GS_SET_XYZ(x1, y2, 0);
        *GS_XYZ2 = GS_SET_XYZ(x2, y2, 0);
    }
    
    // Flush the rendering pipeline
    gs_flush_rendering_pipeline();
    
    debug_log_info("Direct splat rendering complete");
}

/*
 * Set up optimal rendering context for Gaussian splats
 */
void gs_setup_rendering_context(void) {
    debug_log_verbose("Setting up GS rendering context");
    
    // Set primitive type to triangle strip for quads
    gs_set_primitive_type(SPLATSTORM_GS_PRIM_TRISTRIP);
    
    // Enable alpha blending for transparent splats
    *GS_ALPHA_1 = GS_SET_ALPHA(0, 1, 0, 1, 0);  // Source alpha, dest one-minus-alpha
    
    // Configure Z-buffer for depth testing
    *GS_TEST_1 = GS_SET_TEST(1, GS_ATEST_ALWAYS, 0, GS_AFAIL_KEEP, 0, 0, 1, GS_ZTEST_GEQUAL);
    
    // Set up frame buffer
    *GS_FRAME_1 = GS_SET_FRAME(0x0, 640/64, GS_PSM_CT32, 0xFF000000);
    
    // Set up Z-buffer
    *GS_ZBUF_1 = GS_SET_ZBUF(0x8C000, GS_PSMZ_32, 0);
    
    debug_log_verbose("GS rendering context configured");
}



/*
 * Set GS primitive type for rendering
 */
void gs_set_primitive_type(int type) {
    debug_log_verbose("Setting GS primitive type: %d", type);
    
    u64 prim_value = 0;
    
    switch (type) {
        case SPLATSTORM_GS_PRIM_POINT:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_POINT, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_LINE:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_LINE, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_LINESTRIP:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_LINESTRIP, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_TRI:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_TRIANGLE, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_TRISTRIP:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_TRISTRIP, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_TRIFAN:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_TRIFAN, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        case SPLATSTORM_GS_PRIM_SPRITE:
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
        default:
            debug_log_warning("Unknown primitive type %d, using triangle strip", type);
            prim_value = GS_SET_PRIM(GS_PRIM_PRIM_TRISTRIP, 0, 0, 0, 1, 0, 1, 0, 0);
            break;
    }
    
    *GS_PRIM = prim_value;
    debug_log_verbose("GS primitive type set");
}