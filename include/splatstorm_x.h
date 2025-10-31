/*
 * SPLATSTORM X - Master Engine Header (PS2SDK Only)
 * Real PS2 Gaussian Splat Engine using Official PS2SDK
 */

#ifndef SPLATSTORM_X_H
#define SPLATSTORM_X_H

// Include core types first
#include "splatstorm_types.h"

// _EE is defined via Makefile -D_EE flag - COMPLETE IMPLEMENTATION

// COMPLETE IMPLEMENTATION - Full PS2SDK Headers with Macro Compatibility
#include "macro_compatibility.h"
#include <libpad.h>
#include <libmc.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <dmaKit.h>
#include <gsKit.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// COMPLETE IMPLEMENTATION - PS2 type definitions always available

// PS2 basic type definitions for latest toolchain
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

// gsKit is now included through macro_compatibility.h with proper conflict resolution

// Engine Version
#define SPLATSTORM_VERSION_MAJOR 1
#define SPLATSTORM_VERSION_MINOR 0
#define SPLATSTORM_VERSION_PATCH 0

// Engine Constants
#define TARGET_FPS 60
#define MAX_SPLATS 16000
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Return Codes
#define SPLATSTORM_OK 0
#define SPLATSTORM_ERROR_MEMORY -1
#define SPLATSTORM_ERROR_INIT -2
#define SPLATSTORM_ERROR_GS -3
#define SPLATSTORM_ERROR_DMA -4
#define SPLATSTORM_ERROR_VU -5
#define SPLATSTORM_ERROR_ASSET -6
#define SPLATSTORM_ERROR_NOT_INITIALIZED -7
#define SPLATSTORM_ERROR_INVALID_PARAM -8
#define SPLATSTORM_ERROR_OUT_OF_MEMORY -1  // Alias for SPLATSTORM_ERROR_MEMORY
#define SPLATSTORM_ERROR_HARDWARE_INIT -9
#define SPLATSTORM_ERROR_TIMEOUT -10

// PS2 Hardware Definitions (UNCACHED_SEG already defined in kernel.h)

// GS Constants
#define GS_SET_CSR_RESET    0x00000200
#define GS_PSM_32           0x00
#define GS_PSM_24           0x01
#define GS_PSM_16           0x02

// GS Alpha blending constants
#define GS_ALPHA_CS         0
#define GS_ALPHA_CD         1
#define GS_ALPHA_AS         2
#define GS_ALPHA_AD         3

// GS Alpha test constants
#define GS_ATEST_NEVER      0
#define GS_ATEST_ALWAYS     1
#define GS_ATEST_LESS       2
#define GS_ATEST_LEQUAL     3
#define GS_ATEST_EQUAL      4
#define GS_ATEST_GEQUAL     5
#define GS_ATEST_GREATER    6
#define GS_ATEST_NOTEQUAL   7

// GS Alpha fail constants
#define GS_AFAIL_KEEP       0
#define GS_AFAIL_FB_ONLY    1
#define GS_AFAIL_ZB_ONLY    2
#define GS_AFAIL_RGB_ONLY   3

// GS Depth test constants
#define GS_ZTEST_NEVER      0
#define GS_ZTEST_ALWAYS     1
#define GS_ZTEST_GEQUAL     2
#define GS_ZTEST_GREATER    3

// GS Primitive types (use SPLATSTORM prefix to avoid conflicts)
#define SPLATSTORM_GS_PRIM_POINT       0
#define SPLATSTORM_GS_PRIM_LINE        1
#define SPLATSTORM_GS_PRIM_LINESTRIP   2
#define SPLATSTORM_GS_PRIM_TRI         3
#define SPLATSTORM_GS_PRIM_TRISTRIP    4
#define SPLATSTORM_GS_PRIM_TRIFAN      5
#define SPLATSTORM_GS_PRIM_SPRITE      6

// COMPLETE IMPLEMENTATION - GS Register Addresses always available
#define SPLATSTORM_GS_FRAME_1          0x4C
#define SPLATSTORM_GS_FRAME_2          0x4D
#define SPLATSTORM_GS_ZBUF_1           0x4E
#define SPLATSTORM_GS_ALPHA_1          0x42
#define SPLATSTORM_GS_TEST_1           0x47
#define SPLATSTORM_GS_RGBAQ            0x01
#define SPLATSTORM_GS_XYZ2             0x05
#define SPLATSTORM_GS_SCISSOR_1        0x40

// GS Hardware Register Pointers (use SPLATSTORM prefix to avoid conflicts)
#define SPLATSTORM_GS_CSR              ((volatile u32*)0x12001000)
#define SPLATSTORM_GS_DISPFB1          ((volatile u64*)0x12000070)
#define SPLATSTORM_GS_DISPFB2          ((volatile u64*)0x12000080)

// GS Register Access - COMPLETE IMPLEMENTATION
// Use gsKit provided GS_SETREG macros (already defined in gsInit.h)
// GS registers are accessed through GIF packets via packet2 or gsKit functions

// DMA Channel 10 (GIF) Registers
#define D10_MADR            (volatile u32*)0x1000A000
#define D10_QWC             (volatile u32*)0x1000A020

// DMA Channel Constants - provided by gsKit/dmaInit.h
// #define DMA_CHANNEL_GIF     2  // Use gsKit definitions
// #define DMA_CHANNEL_VIF1    1  // Use gsKit definitions  
// #define DMA_CHANNEL_VIF0    0  // Use gsKit definitions
// #define DMA_CHANNEL_SIF2    7  // Use gsKit definitions
#define DMA_CHANNEL_SPR     8
#define DMA_CHANNEL_COUNT   10

// DMA Channel Functions - COMPLETE IMPLEMENTATION
// Full DMA function implementations (implemented in dma_system_complete.c)
// Complete PS2SDK-compatible signatures with full functionality
int dma_channel_initialize(int channel, void *handler, int flags);
void dma_channel_fast_waits(int channel);
int dma_channel_send_normal(int channel, void *data, int qwc, int flags, int spr);
int dma_channel_wait(int channel, int timeout);
int dma_channel_shutdown(int channel, int flags);

// VU0 Register Addresses
#define VU0_STAT            ((volatile u32*)0x10003000)
#define VU0_FBRST           ((volatile u32*)0x10003010)
#define VU0_VF00            ((volatile u32*)0x11004000)
#define VU0_MICRO_MEM       0x11000000

// VU1 Register Addresses  
#define VU1_STAT            ((volatile u32*)0x10003400)
#define VU1_FBRST           ((volatile u32*)0x10003410)
#define VU1_VF00            ((volatile u32*)0x11008000)
#define VU1_MICRO_MEM       0x11008000
#define VU1_DATA_MEM        0x1100C000

// VU Memory Addresses
#define VU0_DATA_MEM        0x11004000
#define VU1_DATA_MEM        0x1100C000

// VU Status Constants
#define VU_STATUS_RUNNING   0x0001
#define VU_STATUS_RESET     0x0002
#define VU_STATUS_STALL     0x0004
#define VU_STATUS_BUSY      0x0008
#define VU_STATUS_ERROR     0x8000

// Memory Pool Base Addresses
#define EE_CODE_BASE        (void*)0x00100000
#define EE_DOUBLE_BUFFER_A  (void*)0x00200000
#define EE_DOUBLE_BUFFER_B  (void*)0x00300000
#define EE_INDEX_BUFFER     (void*)0x00400000
#define EE_MUTATION_STREAM  (void*)0x00500000
#define EE_STACK_HEAP       (void*)0x00600000
#define EE_IOP_MODULES      (void*)0x00700000

// VRAM Pool Base Addresses
#define VRAM_SPLAT_ATLAS    (void*)0x00000000
#define VRAM_OCTREE_MAP     (void*)0x00100000
#define VRAM_LIGHT_PROBE    (void*)0x00200000
#define VRAM_ZBUFFER        (void*)0x00300000
#define VRAM_FRAMEBUFFER    (void*)0x00400000
#define VRAM_GS_CONTEXT     (void*)0x00500000

// Fixed-point math types
typedef s32 fixed16_t;
typedef s16 fixed8_t;
#define FIXED16_ONE     0x10000
#define FIXED8_ONE      0x100
#define FLOAT_TO_FIXED16(f)   ((fixed16_t)((f) * 65536.0f))
#define FLOAT_TO_FIXED8(f)    ((fixed8_t)((f) * 256.0f))
#define FIXED16_TO_FLOAT(f)   ((float)(f) / 65536.0f)
#define FIXED8_TO_FLOAT(f)    ((float)(f) / 256.0f)
#define FIXED16_HALF    0x8000
#define FIXED8_HALF     0x80

// Splat Structure is now defined in splatstorm_types.h

// Compact Splat for VU processing (aligned for DMA)
typedef struct {
    float pos[4];       // Position (x, y, z, w) - w=1.0 for homogeneous coords
    u32 color_packed;   // RGBA packed as 32-bit
    float scale[2];     // Scale (width, height)
} CompactSplat __attribute__((aligned(16)));

// Transformed Splat after VU processing
typedef struct {
    float screen_pos[4]; // Screen position (x, y, z, w)
    u32 color_packed;    // RGBA packed as 32-bit
    float scale[2];      // Transformed scale
    u8 visible;          // Visibility flag
    u8 padding[3];       // Padding for alignment
} TransformedSplat __attribute__((aligned(16)));

// 4x4 Matrix for transformations
typedef struct {
    float m[4][4];
} matrix4 __attribute__((aligned(16)));

// Engine State
typedef struct {
    u32 frame_count;
    u64 frame_start_time;
    u64 frame_end_time;
    float frame_time_ms;
    float fps;
    u32 splat_count;
    u32 visible_splats;
    int last_error;
    char error_message[256];
} engine_state_t;

// FrameProfileData is defined in gaussian_types.h

// Global Engine State
extern engine_state_t g_engine_state;
extern splat_t* scene_data;
extern u32 splat_count;
// graphics_initialized is now static in graphics_enhanced.c

// Core Engine Functions
int splatstorm_init_all_systems(void);
void splatstorm_main_loop(void);
void splatstorm_shutdown_all_systems(void);
void splatstorm_set_error(int error_code, const char* message);
void splatstorm_emergency_shutdown(void);

// Subsystem Initialization Functions
int memory_init(void);
void gs_init(void);
void dma_init(void);
void vu_init(void);
int input_init(void);
int mc_init(void);

// Robust Initialization Wrappers
int gs_init_robust(void);
int dma_init_robust(void);
int vu_init_robust(void);
int input_init_robust(void);
int mc_init_robust(void);

// New Graphics Functions (Simplified PS2SDK)
void gs_clear_screen(void);
void gs_flip_screen(void);
u32 gs_get_screen_width(void);
u32 gs_get_screen_height(void);

// New DMA Functions (Simplified PS2SDK)
void dma_send_chain(void* data, u32 size);
void dma_build_display_list(splat_t* splats, u32 count);

// New VU Functions (Simplified PS2SDK)
void vu_init_programs(void);

// Asset Functions
int asset_load_splats(const char* filename);
int asset_generate_test_scene(u32 count);

// Memory Management
void* splatstorm_alloc_aligned(u32 size, u32 alignment);
void splatstorm_free_aligned(void* ptr);
void memory_dump_stats(void);

// VU Functions
void vu0_reset(void);
void vu1_reset(void);
void vu0_upload_microcode(u32* start, u32* end);
void vu1_upload_microcode(u32* start, u32* end);
// VU program functions moved to VU Functions section below
void vu_kick_culling(void);
void vu_kick_rendering(void);
u32 vu_get_visible_count(void);

// VU Program Symbols (will be linked from VU assembly)
extern u32 splatstorm_x_vu0_start;
extern u32 splatstorm_x_vu0_end;
extern u32 splatstorm_x_vu1_start;
extern u32 splatstorm_x_vu1_end;

// GS Functions (using PS2SDK libgs)
void gs_set_csr(u32 value);
u64 gs_setreg_frame_1(u32 fbp, u32 fbw, u32 psm, u32 fbmsk);
u64 gs_setreg_frame_2(u32 fbp, u32 fbw, u32 psm, u32 fbmsk);
u64 gs_setreg_zbuf_1(u32 zbp, u32 psm, u32 zmsk);
u64 gs_setreg_alpha_1(u32 a, u32 b, u32 c, u32 d, u32 fix);
u64 gs_setreg_test_1(u32 ate, u32 atst, u32 aref, u32 afail, u32 date, u32 datm, u32 zte, u32 ztst);

// Note: GS register functions are provided by gsKit - no need to redeclare

// GS Hardware access macros (use SPLATSTORM prefix to avoid conflicts)
#define SPLATSTORM_GS_SET_CSR(value) (*SPLATSTORM_GS_CSR = (value))

// COMPLETE IMPLEMENTATION - SIF Functions always available
int SifIopReset(const char* arg, int mode);
int SifIopSync(void);
void SifInitRpc(int mode);
// SifInitIopHeap and SifLoadFileInit are provided by PS2SDK
int SifLoadModule(const char* filename, int args_len, const char* args);

// COMPLETE IMPLEMENTATION - IOP SBV Functions provided by PS2SDK
// sbv_patch_disable_prefix_check is provided by PS2SDK

// COMPLETE IMPLEMENTATION - Kernel Functions always available
int DIntr(void);
int EIntr(void);
void ExitThread(void);

// IOP Functions (implemented in iop.c)
int iop_init_modules(void);
void iop_shutdown(void);

// VU Functions
void vu0_start_program(int program_id, void* data);
void vu0_wait_program(void);
void vu1_start_program(int program_id, void* data);

// Fixed-Point Math Functions
void fixed_math_init(void);
void fixed_math_init_tables(void);

// Basic fixed-point arithmetic
fixed16_t fixed16_mul(fixed16_t a, fixed16_t b);
fixed16_t fixed16_div(fixed16_t a, fixed16_t b);
fixed16_t fixed16_abs(fixed16_t value);
fixed16_t fixed16_neg(fixed16_t value);
fixed16_t fixed16_min(fixed16_t a, fixed16_t b);
fixed16_t fixed16_max(fixed16_t a, fixed16_t b);
fixed16_t fixed16_clamp(fixed16_t value, fixed16_t min_val, fixed16_t max_val);

// Trigonometric functions
fixed16_t fixed16_sin(fixed16_t angle);
fixed16_t fixed16_cos(fixed16_t angle);
fixed16_t fixed16_sqrt(fixed16_t value);

// Vector math
void fixed16_vec3_add(fixed16_t* result, const fixed16_t* a, const fixed16_t* b);
void fixed16_vec3_sub(fixed16_t* result, const fixed16_t* a, const fixed16_t* b);
fixed16_t fixed16_vec3_dot(const fixed16_t* a, const fixed16_t* b);
fixed16_t fixed16_vec3_length(const fixed16_t* v);
void fixed16_vec3_normalize(fixed16_t* result, const fixed16_t* v);

// Matrix operations
void fixed16_mat4_identity(fixed16_t* matrix);
void fixed16_mat4_multiply(fixed16_t* result, const fixed16_t* a, const fixed16_t* b);
void fixed16_mat4_vec4_multiply(fixed16_t* result, const fixed16_t* matrix, const fixed16_t* vector);

// Interpolation
fixed16_t fixed16_lerp(fixed16_t a, fixed16_t b, fixed16_t t);
fixed16_t fixed16_smoothstep(fixed16_t edge0, fixed16_t edge1, fixed16_t x);

// Utility functions
fixed16_t float_to_fixed16(float value);
fixed8_t float_to_fixed8(float value);
void float_to_fixed16_array(fixed16_t* dest, const float* src, u32 count);
void fixed16_to_float_array(float* dest, const fixed16_t* src, u32 count);

// Debug functions
void fixed16_print(const char* name, fixed16_t value);
void fixed16_vec3_print(const char* name, const fixed16_t* vec);

// Pad State Structure
typedef struct {
    u8 analog_lx, analog_ly;  // Left analog stick
    u8 analog_rx, analog_ry;  // Right analog stick
    u16 buttons;              // Button state
} pad_state_t;

// Framebuffer System Functions
int framebuffer_init(void);
void framebuffer_shutdown(void);
int framebuffer_init_system(void);
void framebuffer_shutdown_system(void);
void framebuffer_clear_screen(void);
void framebuffer_swap_buffers(void);
void framebuffer_clear(void);
void framebuffer_set_clear_color(u8 r, u8 g, u8 b);
void framebuffer_flip(void);
u16* framebuffer_get_back_buffer(void);
u16* framebuffer_get_front_buffer(void);
void framebuffer_get_dimensions(int* width, int* height);
int framebuffer_set_pixel(int x, int y, u16 color);
u16 framebuffer_get_pixel(int x, int y);
u16 framebuffer_alpha_blend(u16 src, u16 dst, float alpha);
int framebuffer_set_pixel_alpha(int x, int y, u16 color, float alpha);
u16 framebuffer_rgb_to_rgb565(u8 r, u8 g, u8 b);
void framebuffer_rgb565_to_rgb(u16 color, u8* r, u8* g, u8* b);
int framebuffer_is_initialized(void);
u32 framebuffer_get_memory_usage(void);
void framebuffer_fill_rect(int x, int y, int width, int height, u16 color);
void framebuffer_copy_rect(u16* src_buffer, u16* dst_buffer, 
                          int src_x, int src_y, int dst_x, int dst_y, 
                          int width, int height);

// Splat Renderer Functions
void splat_render_list(splat_t* splats, int count, float* view_matrix, float* proj_matrix);
void splat_renderer_get_stats(u32* processed, u32* visible, u32* culled, u32* pixels, float* time_ms);
void splat_renderer_reset_stats(void);
void splat_render_test(float screen_x, float screen_y, float radius, u16 color, float alpha);
int splat_renderer_init(void);
void splat_renderer_shutdown(void);

// Depth Buffer Functions
int depth_buffer_init(void);
void depth_buffer_shutdown(void);
void depth_buffer_clear(void);
void depth_buffer_set_clear_value(u16 value);
int depth_buffer_test(int x, int y, u16 depth);
int depth_buffer_write(int x, int y, u16 depth);
int depth_buffer_test_and_write(int x, int y, u16 depth);
u16 depth_buffer_get(int x, int y);
u16* depth_buffer_get_buffer(void);
void depth_buffer_get_dimensions(int* width, int* height);
int depth_buffer_is_initialized(void);
u32 depth_buffer_get_memory_usage(void);
u16 depth_buffer_float_to_depth(float depth);
float depth_buffer_depth_to_float(u16 depth);
void depth_buffer_fill_rect(int x, int y, int width, int height, u16 depth);
void depth_buffer_copy_rect(u16* src_buffer, u16* dst_buffer,
                           int src_x, int src_y, int dst_x, int dst_y,
                           int width, int height);
void depth_sort_splats(splat_t* splats, int count, int* sorted_indices, int mode);
void depth_get_splat_bounds(splat_t* splats, int count, float* min_depth, float* max_depth);

// Camera System Functions
int camera_init(void);
void camera_shutdown(void);
void camera_update_input(pad_state_t* pad, float delta_time);
void camera_update(void);
float* camera_get_view_matrix(void);
float* camera_get_proj_matrix(void);
float* camera_get_view_proj_matrix(void);
void camera_set_position(float x, float y, float z);
void camera_set_target(float x, float y, float z);
void camera_set_fov(float fov_degrees);
void camera_set_aspect_ratio(float aspect);
void camera_get_position(float* x, float* y, float* z);
void camera_get_target(float* x, float* y, float* z);
int camera_is_initialized(void);

// Memory Management Functions
void* splatstorm_malloc(u32 size);
void splatstorm_free(void* ptr);
int splatstorm_check_memory_integrity(void);
u32 splatstorm_get_memory_usage(void);
u32 splatstorm_get_vram_usage(void);

// Debug Functions
void debug_log_info(const char* format, ...);
void debug_log_error(const char* format, ...);
void debug_log_verbose(const char* format, ...);
void debug_log_warning(const char* format, ...);
void debug_shutdown(void);
void debug_check_stack_overflow(void);
void debug_update_fps(float fps);
void debug_update_memory(u32 memory_usage, u32 vram_usage);
void debug_update_rendering(u32 total_splats, u32 visible_splats, u32 culled_splats);

// Mutation System Functions
void mutation_update(void);

// VU Functions (additional)
void vu1_wait_program(void);

// Input Functions
int input_poll(void);
pad_state_t* input_get_pad_state(void);

// Graphics Test Functions
int test_graphics_init(void);
int test_graphics_run_all(void);
void test_graphics_visual_test(void);
int test_graphics_get_status(void);

// PS2 Hardware Status Structure
typedef struct {
    int hardware_initialized;
    int iop_modules_loaded;
    int dma_channels_initialized;
    u64 gs_csr;
} HardwareStatus;

// PS2 Hardware Initialization
int ps2_hardware_init(void);
int hardware_set_cpu_frequency(u32 frequency);
u32 hardware_get_memory_size(void);
u32 hardware_get_gs_revision(void);
void ps2_hardware_get_status(HardwareStatus *status);
void ps2_hardware_cleanup(void);

// File I/O System Functions
int initialize_file_systems(void);
int file_system_is_ready(void);
int find_file_on_storage(const char* filename, char* full_path, size_t path_size);
int open_file_auto(const char* filename, int flags);
int read_file_data(int fd, void* buffer, size_t size);
int write_file_data(int fd, const void* buffer, size_t size);
int close_file(int fd);
long get_file_size(int fd);
int file_exists(const char* filename);
void file_system_shutdown(void);

// Performance Counter Functions
u64 get_cpu_cycles(void);
u64 get_cpu_cycles_64(void);
u64 timer_us_get64(void);
void performance_init(void);
void performance_frame_start(void);
void performance_frame_end(void);
float performance_get_fps(void);
u64 performance_get_avg_frame_time(void);
float performance_get_cpu_utilization(void);
u32 performance_get_memory_usage(void);
void performance_set_memory_usage(u32 bytes_used);
u32 performance_get_vram_usage(void);
void performance_set_vram_usage(u32 bytes_used);
void performance_print_stats(void);
void performance_reset_stats(void);
void performance_shutdown(void);

// Hardware Detection Functions
int hardware_detect_capabilities(void);
const char* hardware_get_model_name(void);
const char* hardware_get_region(void);
u32 hardware_get_cpu_frequency(void);
u32 hardware_get_bus_frequency(void);
int hardware_is_slim_model(void);
int hardware_has_network_adapter(void);
int hardware_has_hdd(void);
int hardware_has_usb(void);
int hardware_has_firewire(void);
void hardware_detection_shutdown(void);

// Enhanced system structures (forward declarations)
typedef struct {
    bool initialized;
    bool connected;
    char ip_address[16];
    char netmask[16];
    char gateway[16];
    // COMPLETE IMPLEMENTATION - All network statistics fields
    uint64_t init_time;
    uint64_t connect_time;
    uint64_t uptime_ticks;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t sockets_created;
    uint32_t sockets_closed;
    uint32_t connections_established;
    uint32_t connection_failures;
    uint32_t send_errors;
    uint32_t receive_errors;
    int active_sockets;
} network_stats_t;

typedef struct {
    bool initialized;
    bool pad_connected;
    bool keyboard_available;
    bool mouse_available;
    bool pressure_sensitive;
} input_stats_t;

typedef struct {
    bool initialized;
    u32 assets_loaded;
    u32 total_memory;
    u32 memory_kb;
} asset_stats_t;

typedef struct {
    u32 total_splats_processed;
    u32 total_splats_culled;
    u32 average_culling_time_us;
    u32 vu0_utilization_percent;
    u32 dma_transfer_time_us;
} VUCullingStats;

// COMPLETE IMPLEMENTATION - Timer functions
uint64_t splatstorm_timer_get_ticks(void);

// COMPLETE IMPLEMENTATION - Network functions always available
int splatstorm_network_init(void);
void splatstorm_network_shutdown(void);
int splatstorm_network_configure(const char* ip, const char* mask, const char* gw);
bool splatstorm_network_is_connected(void);
const char* splatstorm_network_get_ip(void);
int splatstorm_network_create_socket(void);
int splatstorm_network_connect(int sock, const char* host, int port);
int splatstorm_network_send(int sock, const void* data, size_t size);
int splatstorm_network_receive(int sock, void* buffer, size_t size);
void splatstorm_network_close_socket(int sock);
void splatstorm_network_get_stats(network_stats_t* stats);

// COMPLETE IMPLEMENTATION - Enhanced Input functions always available
int splatstorm_input_enhanced_init(void);
void splatstorm_input_enhanced_shutdown(void);
void splatstorm_input_enhanced_update(void);
bool splatstorm_input_pad_connected(void);
bool splatstorm_input_pad_button_pressed(u16 button);
bool splatstorm_input_pad_button_held(u16 button);
bool splatstorm_input_pad_button_released(u16 button);
void splatstorm_input_pad_get_analog(u8* lx, u8* ly, u8* rx, u8* ry);
u8 splatstorm_input_pad_get_pressure(int button_index);
bool splatstorm_input_keyboard_available(void);
bool splatstorm_input_key_pressed(u8 key);
bool splatstorm_input_key_held(u8 key);
bool splatstorm_input_key_released(u8 key);
bool splatstorm_input_mouse_available(void);
void splatstorm_input_mouse_get_position(int* x, int* y);
void splatstorm_input_mouse_get_delta(int* dx, int* dy);
bool splatstorm_input_mouse_button_pressed(u8 button);
bool splatstorm_input_mouse_button_held(u8 button);
bool splatstorm_input_mouse_button_released(u8 button);
void splatstorm_input_get_stats(input_stats_t* stats);
void splatstorm_input_get_camera_input(float* move_x, float* move_y, float* move_z, 
                                      float* look_x, float* look_y);

// COMPLETE IMPLEMENTATION - Asset Pipeline functions always available
int splatstorm_asset_pipeline_init(void);
void splatstorm_asset_pipeline_shutdown(void);
// COMPLETE IMPLEMENTATION - Graphics Enhanced functions always available
GSTEXTURE* splatstorm_asset_load_texture(const char* filename);
int splatstorm_asset_save_texture(GSTEXTURE* texture, const char* filename);
GSTEXTURE* splatstorm_asset_create_procedural_texture(int width, int height, 
                                                     u32 (*generator)(int x, int y, void* data),
                                                     void* data);
void* splatstorm_asset_load_font(const char* filename, int size);
u32 splatstorm_asset_checkerboard_generator(int x, int y, void* data);
u32 splatstorm_asset_gradient_generator(int x, int y, void* data);
u32 splatstorm_asset_noise_generator(int x, int y, void* data);
int splatstorm_asset_validate_file(const char* filename);
void splatstorm_asset_get_stats(asset_stats_t* stats);
int splatstorm_asset_load_batch(const char** filenames, int count);

// COMPLETE IMPLEMENTATION - Enhanced system includes always available
// Enhanced system includes (AthenaEnv integration - NO JAVASCRIPT, NO AUDIO)
#include "graphics_enhanced.h"

// COMPLETE IMPLEMENTATION - VIF DMA and IOP modules always available
#include "vif_dma.h"

// Enhanced IOP module system
#include "iop_modules.h"

// Additional type definitions for main_complete.c
typedef struct {
    u8 left_stick_x, left_stick_y;    // 0-255, center at 128
    u8 right_stick_x, right_stick_y;  // 0-255, center at 128
    u32 buttons;
    u32 buttons_pressed;
} InputState;

typedef struct {
    bool initialized;
    int pad_state;
    int pad_mode;
    u16 current_buttons;
    u8 analog_lx, analog_ly;
    u8 analog_rx, analog_ry;
    int buffer_head;
} InputDebugInfo;

// Error codes for robust error handling (moved here to avoid circular includes)
typedef enum {
    GAUSSIAN_SUCCESS = 0,
    GAUSSIAN_ERROR_MEMORY_ALLOCATION = -1,
    GAUSSIAN_ERROR_INVALID_PARAMETER = -2,
    GAUSSIAN_ERROR_VU_INITIALIZATION = -3,
    GAUSSIAN_ERROR_TEXTURE_UPLOAD = -4,
    GAUSSIAN_ERROR_NUMERICAL_INSTABILITY = -5,
    GAUSSIAN_ERROR_OVERFLOW = -6,
    GAUSSIAN_ERROR_VU1_TIMEOUT = -7,
    GAUSSIAN_ERROR_DMA_FAILURE = -8,
    GAUSSIAN_ERROR_GS_FAILURE = -9,
    GAUSSIAN_ERROR_UNSUPPORTED_FORMAT = -10,
    GAUSSIAN_ERROR_FILE_NOT_FOUND = -11,
    GAUSSIAN_ERROR_INVALID_FORMAT = -12,
    GAUSSIAN_ERROR_BUSY = -13,
    GAUSSIAN_ERROR_INIT_FAILED = -14,
    GAUSSIAN_ERROR_MODULE_LOAD_FAILED = -15,
    GAUSSIAN_ERROR_FILE_OPEN_FAILED = -16,
    GAUSSIAN_ERROR_FILE_READ_FAILED = -17,
    GAUSSIAN_ERROR_FILE_WRITE_FAILED = -18,
    GAUSSIAN_ERROR_FILE_CLOSE_FAILED = -19,
    GAUSSIAN_ERROR_OUT_OF_MEMORY = -20,
    GAUSSIAN_ERROR_TOO_MANY_PROPERTIES = -21
} GaussianResult;

// Frustum is now defined in splatstorm_types.h

// Input button constants
#define INPUT_BUTTON_L1         0x0004
#define INPUT_BUTTON_R1         0x0008
#define INPUT_BUTTON_L2         0x0001
#define INPUT_BUTTON_R2         0x0002
#define INPUT_BUTTON_SELECT     0x0100
#define INPUT_BUTTON_START      0x0800
#define INPUT_BUTTON_TRIANGLE   0x1000
#define INPUT_BUTTON_SQUARE     0x8000

// Include gaussian types
#include "gaussian_types.h"

// Scene constants
#define MAX_SCENE_SPLATS    MAX_SPLATS

// Additional function declarations for complete implementations
int memory_system_init(void);
void memory_system_cleanup(void);
int memory_pool_create(int type, u32 size, u32 alignment, u32* pool_id);
void* memory_pool_alloc(u32 pool_id, u32 size, u32 alignment, const char* file, u32 line);
#define MEMORY_POOL_ALLOC(pool_id, size) memory_pool_alloc(pool_id, size, 16, __FILE__, __LINE__)
void memory_pool_free(u32 pool_id, void* ptr);
void memory_pool_reset(u32 pool_id);
int input_system_init(void);
void input_system_cleanup(void);
void input_update(InputState* input);
int vu_system_init(void);
void vu_system_cleanup(void);
int vu_load_microcode(void);
GaussianResult dma_system_init(void);
void dma_system_cleanup(void);
int tile_system_init(u32 max_splats);
GaussianResult gs_renderer_init(u32 width, u32 height, u32 psm);
void camera_init_fixed(void* camera);
void camera_set_position_fixed(void* camera, float x, float y, float z);
void camera_set_target_fixed(void* camera, float x, float y, float z);
void camera_update_matrices_fixed(void* camera);
void camera_move_relative_fixed(void* camera, float x, float y, float z);
void camera_rotate_fixed(void* camera, float pitch, float yaw, float roll);
void camera_extract_frustum_fixed(void* camera, Frustum* frustum);
int load_ply_file_fixed(const char* filename, void* splats, u32* count);

// Complete PLY loader functions
GaussianResult load_ply_file(const char* filename, GaussianSplat3D** splats, u32* count);
GaussianResult validate_ply_file(const char* filename, u32* vertex_count);
GaussianResult get_ply_info(const char* filename, PLYFileInfo* info);

// Complete frustum culling functions
GaussianResult init_spatial_grid(const GaussianSplat3D* splats, u32 splat_count);
GaussianResult extract_frustum_planes(const fixed16_t view_proj_matrix[16], void* frustum);
GaussianResult cull_gaussian_splats(const GaussianSplat3D* input_splats, u32 input_count,
                                   const fixed16_t view_proj_matrix[16],
                                   GaussianSplat3D* output_splats, u32* output_count);
GaussianResult get_culling_stats(CullingStats* stats);
bool is_sphere_visible(const fixed16_t center[3], fixed16_t radius, void* frustum_ptr);
void cleanup_frustum_culling(void);
GaussianResult gs_upload_lut_textures(const GaussianLUTs* luts);
int vu_upload_constants(void* camera);
void vu_wait_for_completion(void);
void gs_clear_buffers(u32 color, u32 depth);
void gs_swap_contexts(void);
int vu_process_batch(void* visible_splats, u32 visible_count, void* projected_splats, u32* projected_count);
int process_tiles(void* projected_splats, u32 projected_count, void* camera, void* tile_ranges);
void gs_set_scissor_rect(u32 x, u32 y, u32 width, u32 height);
void gs_disable_scissor(void);
const u32* get_tile_splat_list(u32 tile_id, u32* count);
void gs_render_splat_batch(const GaussianSplat2D* splats, u32 splat_count);
void gs_render_debug_overlay(void);
void gs_enable_debug_mode(bool show_tiles, bool show_centers, u32 overlay_color);
void gs_renderer_cleanup(void);
void tile_system_cleanup(void);

// Profiling System Functions
int profiling_init(void);
void profiling_start_timer(const char* name);
void profiling_end_timer(const char* name);
void profiling_print_results(void);
void profiling_get_data(FrameProfileData* data);
int profiling_system_init(void);
void profile_frame_start(void);
void profile_dma_upload_start(void);
void profile_dma_upload_end(void);
void profile_vu_execute_start(void);
void profile_vu_execute_end(void);
void profile_gs_render_start(void);
void profile_gs_render_end(void);
void profile_frame_end(void);
void profile_set_splat_stats(u32 processed, u32 culled);
void profile_set_overdraw_stats(u32 overdraw_pixels);
void profile_get_frame_data(FrameProfileData* data);
void profiling_get_stats_summary(float* avg_frame_time, float* current_fps, u32* total_frames, u32* avg_splats);
void profiling_end_frame(void);
float profiling_get_frame_time(void);

#endif // SPLATSTORM_X_H
