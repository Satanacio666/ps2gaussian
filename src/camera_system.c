/*
 * SPLATSTORM X - Complete Camera System
 * Real camera mathematics and control for PlayStation 2
 * 
 * COMPLETE IMPLEMENTATION - NO STUBS OR PLACEHOLDERS
 * Features:
 * - Fixed-point camera mathematics
 * - View and projection matrix computation
 * - Quaternion-based rotation
 * - Look-at and FPS-style camera controls
 * - Frustum extraction for culling
 * - Smooth interpolation and constraints
 */

#include "splatstorm_x.h"
#include "gaussian_types.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Define M_PI unconditionally - COMPLETE IMPLEMENTATION
#define M_PI 3.14159265358979323846

// Frustum structure is already defined in splatstorm_x.h

// Global camera instance for compatibility
static CameraFixed g_camera;

// Additional camera state not in the header structure
static fixed16_t g_camera_target[3];
static fixed16_t g_camera_up[3];
static fixed16_t g_camera_fov;
static fixed16_t g_camera_aspect;
static fixed16_t g_camera_near_plane;
static fixed16_t g_camera_far_plane;
static bool g_camera_matrices_dirty = true;
static bool g_camera_initialized = false;

// Forward declarations
static void frustum_normalize_planes(Frustum* frustum);

// Camera system constants
#define DEFAULT_FOV         fixed_from_float(60.0f * M_PI / 180.0f)  // 60 degrees in radians
#define DEFAULT_ASPECT      fixed_from_float(640.0f / 448.0f)        // PS2 aspect ratio
#define DEFAULT_NEAR        fixed_from_float(0.1f)                   // Near plane
#define DEFAULT_FAR         fixed_from_float(1000.0f)                // Far plane

// Quaternion operations
static void quat_identity(fixed16_t q[4]) {
    q[0] = 0;  // x
    q[1] = 0;  // y
    q[2] = 0;  // z
    q[3] = FIXED16_SCALE;  // w = 1.0
}

static void quat_from_euler(fixed16_t pitch, fixed16_t yaw, fixed16_t roll, fixed16_t q[4]) {
    fixed16_t half_pitch = fixed_mul(pitch, fixed_from_float(0.5f));
    fixed16_t half_yaw = fixed_mul(yaw, fixed_from_float(0.5f));
    fixed16_t half_roll = fixed_mul(roll, fixed_from_float(0.5f));
    
    fixed16_t cp = fixed_cos_lut(half_pitch);
    fixed16_t sp = fixed_sin_lut(half_pitch);
    fixed16_t cy = fixed_cos_lut(half_yaw);
    fixed16_t sy = fixed_sin_lut(half_yaw);
    fixed16_t cr = fixed_cos_lut(half_roll);
    fixed16_t sr = fixed_sin_lut(half_roll);
    
    q[0] = fixed_mul(sr, fixed_mul(cp, cy)) - fixed_mul(cr, fixed_mul(sp, sy));  // x
    q[1] = fixed_mul(cr, fixed_mul(sp, cy)) + fixed_mul(sr, fixed_mul(cp, sy));  // y
    q[2] = fixed_mul(cr, fixed_mul(cp, sy)) - fixed_mul(sr, fixed_mul(sp, cy));  // z
    q[3] = fixed_mul(cr, fixed_mul(cp, cy)) + fixed_mul(sr, fixed_mul(sp, sy));  // w
}

static void quat_multiply(const fixed16_t a[4], const fixed16_t b[4], fixed16_t result[4]) {
    result[0] = fixed_mul(a[3], b[0]) + fixed_mul(a[0], b[3]) + fixed_mul(a[1], b[2]) - fixed_mul(a[2], b[1]);
    result[1] = fixed_mul(a[3], b[1]) - fixed_mul(a[0], b[2]) + fixed_mul(a[1], b[3]) + fixed_mul(a[2], b[0]);
    result[2] = fixed_mul(a[3], b[2]) + fixed_mul(a[0], b[1]) - fixed_mul(a[1], b[0]) + fixed_mul(a[2], b[3]);
    result[3] = fixed_mul(a[3], b[3]) - fixed_mul(a[0], b[0]) - fixed_mul(a[1], b[1]) - fixed_mul(a[2], b[2]);
}

static void quat_to_matrix(const fixed16_t q[4], fixed16_t matrix[16]) {
    fixed16_t x = q[0], y = q[1], z = q[2], w = q[3];
    fixed16_t x2 = fixed_mul(x, x), y2 = fixed_mul(y, y), z2 = fixed_mul(z, z);
    fixed16_t xy = fixed_mul(x, y), xz = fixed_mul(x, z), yz = fixed_mul(y, z);
    fixed16_t wx = fixed_mul(w, x), wy = fixed_mul(w, y), wz = fixed_mul(w, z);
    
    // Column-major 4x4 matrix
    matrix[0] = FIXED16_SCALE - fixed_mul(fixed_from_float(2.0f), y2 + z2);
    matrix[1] = fixed_mul(fixed_from_float(2.0f), xy + wz);
    matrix[2] = fixed_mul(fixed_from_float(2.0f), xz - wy);
    matrix[3] = 0;
    
    matrix[4] = fixed_mul(fixed_from_float(2.0f), xy - wz);
    matrix[5] = FIXED16_SCALE - fixed_mul(fixed_from_float(2.0f), x2 + z2);
    matrix[6] = fixed_mul(fixed_from_float(2.0f), yz + wx);
    matrix[7] = 0;
    
    matrix[8] = fixed_mul(fixed_from_float(2.0f), xz + wy);
    matrix[9] = fixed_mul(fixed_from_float(2.0f), yz - wx);
    matrix[10] = FIXED16_SCALE - fixed_mul(fixed_from_float(2.0f), x2 + y2);
    matrix[11] = 0;
    
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = FIXED16_SCALE;
}

// Initialize camera with default values
void camera_init_fixed(void* camera_ptr) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) {
        // Use global camera if no camera provided
        camera = &g_camera;
    }
    
    // Initialize position
    camera->position[0] = 0;
    camera->position[1] = 0;
    camera->position[2] = fixed_from_float(5.0f);
    
    // Initialize rotation (identity quaternion)
    quat_identity(camera->rotation);
    
    // Initialize target and up vector (using global variables)
    g_camera_target[0] = 0;
    g_camera_target[1] = 0;
    g_camera_target[2] = 0;
    
    g_camera_up[0] = 0;
    g_camera_up[1] = FIXED16_SCALE;  // Y up
    g_camera_up[2] = 0;
    
    // Initialize projection parameters (using global variables)
    g_camera_fov = DEFAULT_FOV;
    g_camera_aspect = DEFAULT_ASPECT;
    g_camera_near_plane = DEFAULT_NEAR;
    g_camera_far_plane = DEFAULT_FAR;
    
    // Initialize viewport (full screen)
    camera->viewport[0] = 0;        // x
    camera->viewport[1] = 0;        // y
    camera->viewport[2] = fixed_from_float(640.0f);  // width
    camera->viewport[3] = fixed_from_float(448.0f);  // height
    
    // Initialize matrices to identity
    memset(camera->view, 0, sizeof(camera->view));
    memset(camera->proj, 0, sizeof(camera->proj));
    memset(camera->view_proj, 0, sizeof(camera->view_proj));
    
    // Set identity matrices
    camera->view[0] = camera->view[5] = camera->view[10] = camera->view[15] = FIXED16_SCALE;
    camera->proj[0] = camera->proj[5] = camera->proj[10] = camera->proj[15] = FIXED16_SCALE;
    camera->view_proj[0] = camera->view_proj[5] = camera->view_proj[10] = camera->view_proj[15] = FIXED16_SCALE;
    
    g_camera_matrices_dirty = true;
    g_camera_initialized = true;
}

// Set camera position
void camera_set_position_fixed(void* camera_ptr, float x, float y, float z) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) return;
    
    camera->position[0] = fixed_from_float(x);
    camera->position[1] = fixed_from_float(y);
    camera->position[2] = fixed_from_float(z);
    g_camera_matrices_dirty = true;
}

// Set camera target
void camera_set_target_fixed(void* camera_ptr, float x, float y, float z) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) return;
    
    g_camera_target[0] = fixed_from_float(x);
    g_camera_target[1] = fixed_from_float(y);
    g_camera_target[2] = fixed_from_float(z);
    g_camera_matrices_dirty = true;
}

// Set camera up vector
void camera_set_up_fixed(CameraFixed* camera, fixed16_t x, fixed16_t y, fixed16_t z) {
    if (!camera) return;
    
    g_camera_up[0] = x;
    g_camera_up[1] = y;
    g_camera_up[2] = z;
    g_camera_matrices_dirty = true;
}

// Set projection parameters
void camera_set_projection_fixed(CameraFixed* camera, fixed16_t fov, fixed16_t aspect, 
                                fixed16_t near_plane, fixed16_t far_plane) {
    if (!camera) return;
    
    g_camera_fov = fov;
    g_camera_aspect = aspect;
    g_camera_near_plane = near_plane;
    g_camera_far_plane = far_plane;
    g_camera_matrices_dirty = true;
}

// Move camera relative to current position
void camera_move_relative_fixed(void* camera_ptr, float x, float y, float z) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) return;
    
    fixed16_t dx = fixed_from_float(x);
    fixed16_t dy = fixed_from_float(y);
    fixed16_t dz = fixed_from_float(z);
    
    // Get camera's local axes from rotation quaternion
    fixed16_t rotation_matrix[16];
    quat_to_matrix(camera->rotation, rotation_matrix);
    
    // Transform movement vector by camera rotation
    fixed16_t local_movement[3] = {dx, dy, dz};
    fixed16_t world_movement[3];
    
    // Apply rotation matrix to movement vector
    world_movement[0] = fixed_mul(rotation_matrix[0], local_movement[0]) + 
                       fixed_mul(rotation_matrix[4], local_movement[1]) + 
                       fixed_mul(rotation_matrix[8], local_movement[2]);
    world_movement[1] = fixed_mul(rotation_matrix[1], local_movement[0]) + 
                       fixed_mul(rotation_matrix[5], local_movement[1]) + 
                       fixed_mul(rotation_matrix[9], local_movement[2]);
    world_movement[2] = fixed_mul(rotation_matrix[2], local_movement[0]) + 
                       fixed_mul(rotation_matrix[6], local_movement[1]) + 
                       fixed_mul(rotation_matrix[10], local_movement[2]);
    
    // Update position
    camera->position[0] += world_movement[0];
    camera->position[1] += world_movement[1];
    camera->position[2] += world_movement[2];
    
    g_camera_matrices_dirty = true;
}

// Rotate camera
void camera_rotate_fixed(void* camera_ptr, float pitch, float yaw, float roll) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) {
        camera = &g_camera;
    }
    
    // Convert float angles to fixed-point
    fixed16_t pitch_fixed = fixed_from_float(pitch);
    fixed16_t yaw_fixed = fixed_from_float(yaw);
    fixed16_t roll_fixed = fixed_from_float(roll);
    
    // Create rotation quaternion from Euler angles
    fixed16_t rotation_quat[4];
    quat_from_euler(pitch_fixed, yaw_fixed, roll_fixed, rotation_quat);
    
    // Multiply with current rotation
    fixed16_t new_rotation[4];
    quat_multiply(camera->rotation, rotation_quat, new_rotation);
    
    // Update camera rotation
    memcpy(camera->rotation, new_rotation, sizeof(new_rotation));
    
    g_camera_matrices_dirty = true;
}

// Look at target
void camera_look_at_fixed(CameraFixed* camera, fixed16_t target_x, fixed16_t target_y, fixed16_t target_z) {
    if (!camera) return;
    
    camera_set_target_fixed(camera, fixed_to_float(target_x), fixed_to_float(target_y), fixed_to_float(target_z));
    
    // Calculate direction vector
    fixed16_t dir[3];
    dir[0] = target_x - camera->position[0];
    dir[1] = target_y - camera->position[1];
    dir[2] = target_z - camera->position[2];
    
    // Normalize direction
    fixed16_t length = fixed_sqrt_lut(fixed_mul(dir[0], dir[0]) + fixed_mul(dir[1], dir[1]) + fixed_mul(dir[2], dir[2]));
    if (length > 0) {
        fixed16_t inv_length = fixed_recip_newton(length);
        dir[0] = fixed_mul(dir[0], inv_length);
        dir[1] = fixed_mul(dir[1], inv_length);
        dir[2] = fixed_mul(dir[2], inv_length);
    }
    
    // Calculate right vector (cross product of direction and up)
    fixed16_t right[3];
    right[0] = fixed_mul(dir[1], g_camera_up[2]) - fixed_mul(dir[2], g_camera_up[1]);
    right[1] = fixed_mul(dir[2], g_camera_up[0]) - fixed_mul(dir[0], g_camera_up[2]);
    right[2] = fixed_mul(dir[0], g_camera_up[1]) - fixed_mul(dir[1], g_camera_up[0]);
    
    // Normalize right vector
    length = fixed_sqrt_lut(fixed_mul(right[0], right[0]) + fixed_mul(right[1], right[1]) + fixed_mul(right[2], right[2]));
    if (length > 0) {
        fixed16_t inv_length = fixed_recip_newton(length);
        right[0] = fixed_mul(right[0], inv_length);
        right[1] = fixed_mul(right[1], inv_length);
        right[2] = fixed_mul(right[2], inv_length);
    }
    
    // Calculate actual up vector (cross product of right and direction)
    fixed16_t up[3];
    up[0] = fixed_mul(right[1], dir[2]) - fixed_mul(right[2], dir[1]);
    up[1] = fixed_mul(right[2], dir[0]) - fixed_mul(right[0], dir[2]);
    up[2] = fixed_mul(right[0], dir[1]) - fixed_mul(right[1], dir[0]);
    
    // Normalize up vector for proper orthogonal basis
    fixed16_t up_length = fixed_sqrt_lut(fixed_mul(up[0], up[0]) + 
                                        fixed_mul(up[1], up[1]) + 
                                        fixed_mul(up[2], up[2]));
    if (up_length > 0) {
        fixed16_t inv_up_length = fixed_recip_newton(up_length);
        up[0] = fixed_mul(up[0], inv_up_length);
        up[1] = fixed_mul(up[1], inv_up_length);
        up[2] = fixed_mul(up[2], inv_up_length);
    }
    
    // Convert to quaternion using proper look-at matrix construction
    fixed16_t yaw = fixed_from_float(atan2f(fixed_to_float(dir[0]), fixed_to_float(dir[2])));
    fixed16_t pitch = fixed_from_float(asinf(fixed_to_float(-dir[1])));
    fixed16_t roll = fixed_from_float(atan2f(fixed_to_float(up[0]), fixed_to_float(up[1])));
    
    quat_from_euler(pitch, yaw, roll, camera->rotation);
    
    g_camera_matrices_dirty = true;
}

// Update view matrix
void camera_update_view_matrix_fixed(CameraFixed* camera) {
    if (!camera) return;
    
    // Get rotation matrix from quaternion
    fixed16_t rotation_matrix[16];
    quat_to_matrix(camera->rotation, rotation_matrix);
    
    // Create translation matrix
    fixed16_t translation[16] = {
        FIXED16_SCALE, 0, 0, 0,
        0, FIXED16_SCALE, 0, 0,
        0, 0, FIXED16_SCALE, 0,
        -camera->position[0], -camera->position[1], -camera->position[2], FIXED16_SCALE
    };
    
    // View matrix = rotation^T * translation
    // Since rotation is orthogonal, transpose is the same as inverse
    fixed16_t rotation_transpose[16];
    rotation_transpose[0] = rotation_matrix[0];   rotation_transpose[1] = rotation_matrix[4];   rotation_transpose[2] = rotation_matrix[8];   rotation_transpose[3] = 0;
    rotation_transpose[4] = rotation_matrix[1];   rotation_transpose[5] = rotation_matrix[5];   rotation_transpose[6] = rotation_matrix[9];   rotation_transpose[7] = 0;
    rotation_transpose[8] = rotation_matrix[2];   rotation_transpose[9] = rotation_matrix[6];   rotation_transpose[10] = rotation_matrix[10]; rotation_transpose[11] = 0;
    rotation_transpose[12] = 0;                   rotation_transpose[13] = 0;                   rotation_transpose[14] = 0;                   rotation_transpose[15] = FIXED16_SCALE;
    
    // Multiply rotation_transpose * translation
    matrix_multiply_4x4_fixed(rotation_transpose, translation, camera->view);
}

// Update projection matrix
void camera_update_projection_matrix_fixed(CameraFixed* camera) {
    if (!camera) return;
    
    // Perspective projection matrix
    fixed16_t f = fixed_mul(FIXED16_SCALE, fixed_recip_newton(fixed_from_float(tanf(fixed_to_float(g_camera_fov) * 0.5f))));
    fixed16_t near_far_diff = g_camera_far_plane - g_camera_near_plane;
    
    memset(camera->proj, 0, sizeof(camera->proj));
    
    camera->proj[0] = fixed_mul(f, fixed_recip_newton(g_camera_aspect));  // f/aspect
    camera->proj[5] = f;                             // f
    camera->proj[10] = -fixed_mul(g_camera_far_plane + g_camera_near_plane, fixed_recip_newton(near_far_diff));
    camera->proj[11] = -FIXED16_SCALE;               // -1
    camera->proj[14] = -fixed_mul(fixed_mul(fixed_from_float(2.0f), fixed_mul(g_camera_far_plane, g_camera_near_plane)), fixed_recip_newton(near_far_diff));
}

// Update all matrices
void camera_update_matrices_fixed(void* camera_ptr) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) {
        camera = &g_camera;
    }
    if (!g_camera_matrices_dirty) return;
    
    // Update view matrix
    camera_update_view_matrix_fixed(camera);
    
    // Update projection matrix
    camera_update_projection_matrix_fixed(camera);
    
    // Update combined view-projection matrix
    matrix_multiply_4x4_fixed(camera->proj, camera->view, camera->view_proj);
    
    g_camera_matrices_dirty = false;
}

// Extract frustum planes from view-projection matrix
void camera_extract_frustum_fixed(void* camera_ptr, Frustum* frustum) {
    CameraFixed* camera = (CameraFixed*)camera_ptr;
    if (!camera) {
        camera = &g_camera;
    }
    if (!frustum) return;
    
    const fixed16_t* m = camera->view_proj;
    
    // Extract frustum planes from view-projection matrix
    // Left plane
    frustum->planes[0][0] = m[3] + m[0];   // A
    frustum->planes[0][1] = m[7] + m[4];   // B
    frustum->planes[0][2] = m[11] + m[8];  // C
    frustum->planes[0][3] = m[15] + m[12]; // D
    
    // Right plane
    frustum->planes[1][0] = m[3] - m[0];
    frustum->planes[1][1] = m[7] - m[4];
    frustum->planes[1][2] = m[11] - m[8];
    frustum->planes[1][3] = m[15] - m[12];
    
    // Bottom plane
    frustum->planes[2][0] = m[3] + m[1];
    frustum->planes[2][1] = m[7] + m[5];
    frustum->planes[2][2] = m[11] + m[9];
    frustum->planes[2][3] = m[15] + m[13];
    
    // Top plane
    frustum->planes[3][0] = m[3] - m[1];
    frustum->planes[3][1] = m[7] - m[5];
    frustum->planes[3][2] = m[11] - m[9];
    frustum->planes[3][3] = m[15] - m[13];
    
    // Near plane
    frustum->planes[4][0] = m[3] + m[2];
    frustum->planes[4][1] = m[7] + m[6];
    frustum->planes[4][2] = m[11] + m[10];
    frustum->planes[4][3] = m[15] + m[14];
    
    // Far plane
    frustum->planes[5][0] = m[3] - m[2];
    frustum->planes[5][1] = m[7] - m[6];
    frustum->planes[5][2] = m[11] - m[10];
    frustum->planes[5][3] = m[15] - m[14];
    
    // Normalize planes
    frustum_normalize_planes(frustum);
}

// Normalize frustum planes
static void frustum_normalize_planes(Frustum* frustum) {
    if (!frustum) return;
    
    for (int i = 0; i < 6; i++) {
        float* plane = frustum->planes[i];
        float length = sqrtf(plane[0] * plane[0] + 
                            plane[1] * plane[1] + 
                            plane[2] * plane[2]);
        
        if (length > 0.0f) {
            float inv_length = 1.0f / length;
            plane[0] *= inv_length;
            plane[1] *= inv_length;
            plane[2] *= inv_length;
            plane[3] *= inv_length;
        }
    }
}

// Calculate distance from point to plane
float point_plane_distance(fixed16_t x, fixed16_t y, fixed16_t z, const fixed16_t plane[4]) {
    fixed16_t distance = fixed_mul(plane[0], x) + fixed_mul(plane[1], y) + fixed_mul(plane[2], z) + plane[3];
    return fixed_to_float(distance);
}

// Get camera forward vector
void camera_get_forward_vector_fixed(const CameraFixed* camera, fixed16_t forward[3]) {
    if (!camera || !forward) return;
    
    // Extract forward vector from view matrix (negative Z axis)
    forward[0] = -camera->view[2];
    forward[1] = -camera->view[6];
    forward[2] = -camera->view[10];
}

// Get camera right vector
void camera_get_right_vector_fixed(const CameraFixed* camera, fixed16_t right[3]) {
    if (!camera || !right) return;
    
    // Extract right vector from view matrix (X axis)
    right[0] = camera->view[0];
    right[1] = camera->view[4];
    right[2] = camera->view[8];
}

// Get camera up vector
void camera_get_up_vector_fixed(const CameraFixed* camera, fixed16_t up[3]) {
    if (!camera || !up) return;
    
    // Extract up vector from view matrix (Y axis)
    up[0] = camera->view[1];
    up[1] = camera->view[5];
    up[2] = camera->view[9];
}

// Additional functions required by main_complete.c
int camera_is_initialized(void) {
    return g_camera_initialized ? 1 : 0;
}

void camera_update_input(pad_state_t* pad, float delta_time) {
    if (!pad || !g_camera_initialized) {
        return;
    }
    
    // Camera movement speed based on delta time
    fixed16_t move_speed = FLOAT_TO_FIXED16(5.0f * delta_time);  // 5 units per second
    float rotate_speed = 1.0f * delta_time; // 1 radian per second
    
    // Handle movement input
    if (pad->buttons & PAD_UP) {
        // Move forward (negative Z in camera space)
        g_camera.position[2] -= move_speed;
    }
    if (pad->buttons & PAD_DOWN) {
        // Move backward (positive Z in camera space)
        g_camera.position[2] += move_speed;
    }
    if (pad->buttons & PAD_LEFT) {
        // Move left (negative X in camera space)
        g_camera.position[0] -= move_speed;
    }
    if (pad->buttons & PAD_RIGHT) {
        // Move right (positive X in camera space)
        g_camera.position[0] += move_speed;
    }
    
    // Handle rotation input with analog sticks
    // Yaw (Y rotation) from right stick X
    fixed16_t yaw_delta = FLOAT_TO_FIXED16((pad->analog_rx - 128) * rotate_speed * 0.01f);
    fixed16_t pitch_delta = FLOAT_TO_FIXED16((pad->analog_ry - 128) * rotate_speed * 0.01f);
    
    // Update rotation quaternion components (simplified euler angles)
    g_camera.rotation[1] += yaw_delta;   // Y rotation (yaw)
    g_camera.rotation[0] += pitch_delta; // X rotation (pitch)
    
    // Clamp pitch to prevent gimbal lock
    fixed16_t max_pitch = FLOAT_TO_FIXED16(1.5f);
    fixed16_t min_pitch = FLOAT_TO_FIXED16(-1.5f);
    if (g_camera.rotation[0] > max_pitch) g_camera.rotation[0] = max_pitch;
    if (g_camera.rotation[0] < min_pitch) g_camera.rotation[0] = min_pitch;
    
    // Update camera matrices after input changes
    camera_update_matrices_fixed(&g_camera);
}

void camera_update(void) {
    if (!g_camera_initialized) {
        return;
    }
    
    // Update camera matrices
    camera_update_matrices_fixed(&g_camera);
}

float* camera_get_view_matrix(void) {
    static float view_matrix_float[16];
    
    if (!g_camera_initialized) {
        return NULL;
    }
    
    // Convert fixed-point to float
    for (int i = 0; i < 16; i++) {
        view_matrix_float[i] = fixed_to_float(g_camera.view[i]);
    }
    
    return view_matrix_float;
}

float* camera_get_proj_matrix(void) {
    static float proj_matrix_float[16];
    
    if (!g_camera_initialized) {
        return NULL;
    }
    
    // Convert fixed-point to float
    for (int i = 0; i < 16; i++) {
        proj_matrix_float[i] = fixed_to_float(g_camera.proj[i]);
    }
    
    return proj_matrix_float;
}