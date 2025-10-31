/*
 * SPLATSTORM X - Complete Frustum Culling Implementation
 * Production-ready frustum culling with flat spatial grid and VU0 optimization
 * Target: <3ms for 16,000 splats with temporal coherence
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splatstorm_x.h"
#include "gaussian_types.h"
#include <math.h>
#include <tamtypes.h>

// Frustum culling configuration
#define SPATIAL_GRID_SIZE_X     8
#define SPATIAL_GRID_SIZE_Y     8  
#define SPATIAL_GRID_SIZE_Z     8
#define TOTAL_GRID_CELLS        (SPATIAL_GRID_SIZE_X * SPATIAL_GRID_SIZE_Y * SPATIAL_GRID_SIZE_Z)
#define MAX_SPLATS_PER_CELL     512
#define VU0_BATCH_SIZE          128
#define VISIBILITY_HISTORY_BITS 8

// Frustum plane structure (Q16.16 fixed-point)
typedef struct {
    fixed16_t normal[3];        // Plane normal vector
    fixed16_t distance;         // Distance from origin
} FrustumPlane;

// Complete frustum structure
typedef struct {
    FrustumPlane planes[6];     // Left, right, top, bottom, near, far
    fixed16_t bounds_min[3];    // Frustum bounding box min
    fixed16_t bounds_max[3];    // Frustum bounding box max
} FrustumInternal;

// Spatial grid cell
typedef struct {
    fixed16_t bounds_min[3];    // Cell bounding box min
    fixed16_t bounds_max[3];    // Cell bounding box max
    u32 splat_indices[MAX_SPLATS_PER_CELL];  // Indices of splats in this cell
    u32 splat_count;            // Number of splats in cell
    bool is_visible;            // Cell visibility flag (for hierarchical culling)
    u64 last_visible_frame;     // Last frame this cell was visible
} SpatialGridCell;

// Spatial grid structure
typedef struct {
    SpatialGridCell cells[TOTAL_GRID_CELLS];
    fixed16_t world_min[3];     // World space bounds
    fixed16_t world_max[3];
    fixed16_t cell_size[3];     // Size of each cell
    u32 total_splats;           // Total splats in grid
    bool initialized;           // Grid initialization flag
} SpatialGrid;

// Visibility history for temporal coherence
typedef struct {
    u8 history[MAX_SPLATS_PER_SCENE];  // 8-bit visibility history per splat
    u64 frame_number;                  // Current frame number
} VisibilityHistory;

// Global culling state
static SpatialGrid g_spatial_grid = {0};
static VisibilityHistory g_visibility_history = {0};
static u64 g_current_frame = 0;

// Fixed-point math helpers
static inline fixed16_t fixed16_dot3(const fixed16_t a[3], const fixed16_t b[3]) {
    return fixed_mul(a[0], b[0]) + fixed_mul(a[1], b[1]) + fixed_mul(a[2], b[2]);
}

static inline fixed16_t fixed16_length3(const fixed16_t v[3]) {
    fixed16_t dot = fixed16_dot3(v, v);
    return fixed16_sqrt(dot);  // Assuming fixed16_sqrt is implemented
}

// Calculate splat radius from covariance matrix (approximate)
static fixed16_t calculate_splat_radius(const GaussianSplat3D* splat) {
    // Approximate radius as 3 * sqrt(max eigenvalue)
    // For 3x3 covariance matrix, max eigenvalue is approximately the largest diagonal element
    fixed8_t max_cov = splat->cov_mant[0];  // cov[0][0]
    if (splat->cov_mant[4] > max_cov) max_cov = splat->cov_mant[4];  // cov[1][1]
    if (splat->cov_mant[8] > max_cov) max_cov = splat->cov_mant[8];  // cov[2][2]
    
    // Convert Q8.8 to Q16.16 and take square root
    fixed16_t max_cov_16 = (fixed16_t)max_cov << 8;
    return fixed_mul(fixed_from_float(3.0f), fixed16_sqrt(max_cov_16));
}

// Point-plane distance test
static inline fixed16_t point_plane_distance(const fixed16_t point[3], const FrustumPlane* plane) {
    return fixed16_dot3(point, plane->normal) + plane->distance;
}

// Sphere-frustum intersection test
static bool sphere_intersects_frustum(const fixed16_t center[3], fixed16_t radius, const FrustumInternal* frustum) {
    for (int i = 0; i < 6; i++) {
        fixed16_t distance = point_plane_distance(center, &frustum->planes[i]);
        if (distance < -radius) {
            return false;  // Sphere is completely outside this plane
        }
    }
    return true;  // Sphere intersects or is inside frustum
}

// AABB-frustum intersection test
static bool aabb_intersects_frustum(const fixed16_t min_bounds[3], const fixed16_t max_bounds[3], const FrustumInternal* frustum) {
    for (int i = 0; i < 6; i++) {
        const FrustumPlane* plane = &frustum->planes[i];
        
        // Find the positive vertex (farthest along plane normal)
        fixed16_t positive_vertex[3];
        positive_vertex[0] = (plane->normal[0] >= 0) ? max_bounds[0] : min_bounds[0];
        positive_vertex[1] = (plane->normal[1] >= 0) ? max_bounds[1] : min_bounds[1];
        positive_vertex[2] = (plane->normal[2] >= 0) ? max_bounds[2] : min_bounds[2];
        
        // If positive vertex is outside plane, AABB is completely outside
        if (point_plane_distance(positive_vertex, plane) < 0) {
            return false;
        }
    }
    return true;
}

// Initialize spatial grid
GaussianResult init_spatial_grid(const GaussianSplat3D* splats, u32 splat_count) {
    if (!splats || splat_count == 0) {
        return GAUSSIAN_ERROR_INVALID_PARAMETER;
    }
    
    // Calculate world bounds
    g_spatial_grid.world_min[0] = g_spatial_grid.world_max[0] = splats[0].pos[0];
    g_spatial_grid.world_min[1] = g_spatial_grid.world_max[1] = splats[0].pos[1];
    g_spatial_grid.world_min[2] = g_spatial_grid.world_max[2] = splats[0].pos[2];
    
    for (u32 i = 1; i < splat_count; i++) {
        for (int j = 0; j < 3; j++) {
            if (splats[i].pos[j] < g_spatial_grid.world_min[j]) {
                g_spatial_grid.world_min[j] = splats[i].pos[j];
            }
            if (splats[i].pos[j] > g_spatial_grid.world_max[j]) {
                g_spatial_grid.world_max[j] = splats[i].pos[j];
            }
        }
    }
    
    // Add padding to avoid edge cases
    fixed16_t padding = fixed_from_float(1.0f);
    for (int i = 0; i < 3; i++) {
        g_spatial_grid.world_min[i] -= padding;
        g_spatial_grid.world_max[i] += padding;
        g_spatial_grid.cell_size[i] = (g_spatial_grid.world_max[i] - g_spatial_grid.world_min[i]) / 8;
    }
    
    // Initialize cells
    for (int z = 0; z < SPATIAL_GRID_SIZE_Z; z++) {
        for (int y = 0; y < SPATIAL_GRID_SIZE_Y; y++) {
            for (int x = 0; x < SPATIAL_GRID_SIZE_X; x++) {
                int cell_index = z * SPATIAL_GRID_SIZE_Y * SPATIAL_GRID_SIZE_X + y * SPATIAL_GRID_SIZE_X + x;
                SpatialGridCell* cell = &g_spatial_grid.cells[cell_index];
                
                // Calculate cell bounds
                cell->bounds_min[0] = g_spatial_grid.world_min[0] + fixed_mul(fixed_from_int(x), g_spatial_grid.cell_size[0]);
                cell->bounds_min[1] = g_spatial_grid.world_min[1] + fixed_mul(fixed_from_int(y), g_spatial_grid.cell_size[1]);
                cell->bounds_min[2] = g_spatial_grid.world_min[2] + fixed_mul(fixed_from_int(z), g_spatial_grid.cell_size[2]);
                
                cell->bounds_max[0] = cell->bounds_min[0] + g_spatial_grid.cell_size[0];
                cell->bounds_max[1] = cell->bounds_min[1] + g_spatial_grid.cell_size[1];
                cell->bounds_max[2] = cell->bounds_min[2] + g_spatial_grid.cell_size[2];
                
                cell->splat_count = 0;
                cell->is_visible = false;
                cell->last_visible_frame = 0;
            }
        }
    }
    
    // Assign splats to cells
    for (u32 i = 0; i < splat_count; i++) {
        // Calculate cell coordinates
        int cell_x = fixed_to_int((splats[i].pos[0] - g_spatial_grid.world_min[0]) / g_spatial_grid.cell_size[0]);
        int cell_y = fixed_to_int((splats[i].pos[1] - g_spatial_grid.world_min[1]) / g_spatial_grid.cell_size[1]);
        int cell_z = fixed_to_int((splats[i].pos[2] - g_spatial_grid.world_min[2]) / g_spatial_grid.cell_size[2]);
        
        // Clamp to grid bounds
        cell_x = (cell_x < 0) ? 0 : (cell_x >= SPATIAL_GRID_SIZE_X) ? SPATIAL_GRID_SIZE_X - 1 : cell_x;
        cell_y = (cell_y < 0) ? 0 : (cell_y >= SPATIAL_GRID_SIZE_Y) ? SPATIAL_GRID_SIZE_Y - 1 : cell_y;
        cell_z = (cell_z < 0) ? 0 : (cell_z >= SPATIAL_GRID_SIZE_Z) ? SPATIAL_GRID_SIZE_Z - 1 : cell_z;
        
        int cell_index = cell_z * SPATIAL_GRID_SIZE_Y * SPATIAL_GRID_SIZE_X + cell_y * SPATIAL_GRID_SIZE_X + cell_x;
        SpatialGridCell* cell = &g_spatial_grid.cells[cell_index];
        
        // Add splat to cell if there's room
        if (cell->splat_count < MAX_SPLATS_PER_CELL) {
            cell->splat_indices[cell->splat_count] = i;
            cell->splat_count++;
        }
    }
    
    g_spatial_grid.total_splats = splat_count;
    g_spatial_grid.initialized = true;
    
    return GAUSSIAN_SUCCESS;
}

// Extract frustum planes from camera matrices
GaussianResult extract_frustum_planes(const fixed16_t view_proj_matrix[16], void* frustum_ptr) {
    FrustumInternal* frustum = (FrustumInternal*)frustum_ptr;
    if (!view_proj_matrix || !frustum) {
        return GAUSSIAN_ERROR_INVALID_PARAMETER;
    }
    
    // Extract frustum planes from view-projection matrix
    // Left plane: row4 + row1
    frustum->planes[0].normal[0] = view_proj_matrix[3] + view_proj_matrix[0];
    frustum->planes[0].normal[1] = view_proj_matrix[7] + view_proj_matrix[4];
    frustum->planes[0].normal[2] = view_proj_matrix[11] + view_proj_matrix[8];
    frustum->planes[0].distance = view_proj_matrix[15] + view_proj_matrix[12];
    
    // Right plane: row4 - row1
    frustum->planes[1].normal[0] = view_proj_matrix[3] - view_proj_matrix[0];
    frustum->planes[1].normal[1] = view_proj_matrix[7] - view_proj_matrix[4];
    frustum->planes[1].normal[2] = view_proj_matrix[11] - view_proj_matrix[8];
    frustum->planes[1].distance = view_proj_matrix[15] - view_proj_matrix[12];
    
    // Top plane: row4 - row2
    frustum->planes[2].normal[0] = view_proj_matrix[3] - view_proj_matrix[1];
    frustum->planes[2].normal[1] = view_proj_matrix[7] - view_proj_matrix[5];
    frustum->planes[2].normal[2] = view_proj_matrix[11] - view_proj_matrix[9];
    frustum->planes[2].distance = view_proj_matrix[15] - view_proj_matrix[13];
    
    // Bottom plane: row4 + row2
    frustum->planes[3].normal[0] = view_proj_matrix[3] + view_proj_matrix[1];
    frustum->planes[3].normal[1] = view_proj_matrix[7] + view_proj_matrix[5];
    frustum->planes[3].normal[2] = view_proj_matrix[11] + view_proj_matrix[9];
    frustum->planes[3].distance = view_proj_matrix[15] + view_proj_matrix[13];
    
    // Near plane: row4 + row3
    frustum->planes[4].normal[0] = view_proj_matrix[3] + view_proj_matrix[2];
    frustum->planes[4].normal[1] = view_proj_matrix[7] + view_proj_matrix[6];
    frustum->planes[4].normal[2] = view_proj_matrix[11] + view_proj_matrix[10];
    frustum->planes[4].distance = view_proj_matrix[15] + view_proj_matrix[14];
    
    // Far plane: row4 - row3
    frustum->planes[5].normal[0] = view_proj_matrix[3] - view_proj_matrix[2];
    frustum->planes[5].normal[1] = view_proj_matrix[7] - view_proj_matrix[6];
    frustum->planes[5].normal[2] = view_proj_matrix[11] - view_proj_matrix[10];
    frustum->planes[5].distance = view_proj_matrix[15] - view_proj_matrix[14];
    
    // Normalize plane equations
    for (int i = 0; i < 6; i++) {
        fixed16_t length = fixed16_length3(frustum->planes[i].normal);
        if (length > 0) {
            fixed16_t inv_length = fixed16_div(fixed_from_float(1.0f), length);
            frustum->planes[i].normal[0] = fixed_mul(frustum->planes[i].normal[0], inv_length);
            frustum->planes[i].normal[1] = fixed_mul(frustum->planes[i].normal[1], inv_length);
            frustum->planes[i].normal[2] = fixed_mul(frustum->planes[i].normal[2], inv_length);
            frustum->planes[i].distance = fixed_mul(frustum->planes[i].distance, inv_length);
        }
    }
    
    return GAUSSIAN_SUCCESS;
}

// Update visibility history
static void update_visibility_history(u32 splat_index, bool is_visible) {
    if (splat_index >= MAX_SPLATS_PER_SCENE) return;
    
    // Shift history left and add new bit
    g_visibility_history.history[splat_index] <<= 1;
    if (is_visible) {
        g_visibility_history.history[splat_index] |= 1;
    }
}

// Check if splat has temporal coherence (visible for multiple frames)
static bool has_temporal_coherence(u32 splat_index) {
    if (splat_index >= MAX_SPLATS_PER_SCENE) return false;
    
    u8 history = g_visibility_history.history[splat_index];
    
    // Check if visible for last 3 frames
    return (history & 0x07) == 0x07;
}

// VU0 batch processing for frustum culling (simplified - actual implementation would use VU0 assembly)
static void vu0_cull_batch(const GaussianSplat3D* splats, const u32* indices, u32 count, 
                          const FrustumInternal* frustum, bool* results) {
    // This would be implemented in VU0 microcode for optimal performance
    // For now, implement in C with SIMD-friendly operations
    
    for (u32 i = 0; i < count; i++) {
        u32 splat_idx = indices[i];
        const GaussianSplat3D* splat = &splats[splat_idx];
        
        // Calculate splat radius
        fixed16_t radius = calculate_splat_radius(splat);
        
        // Test against all frustum planes
        bool visible = true;
        for (int plane = 0; plane < 6; plane++) {
            fixed16_t distance = point_plane_distance(splat->pos, &frustum->planes[plane]);
            if (distance < -radius) {
                visible = false;
                break;
            }
        }
        
        results[i] = visible;
    }
}

// Main frustum culling function
GaussianResult cull_gaussian_splats(const GaussianSplat3D* input_splats, u32 input_count,
                                   const fixed16_t view_proj_matrix[16],
                                   GaussianSplat3D* output_splats, u32* output_count) {
    if (!input_splats || !view_proj_matrix || !output_splats || !output_count) {
        return GAUSSIAN_ERROR_INVALID_PARAMETER;
    }
    
    // Initialize spatial grid if not done
    if (!g_spatial_grid.initialized) {
        GaussianResult result = init_spatial_grid(input_splats, input_count);
        if (result != GAUSSIAN_SUCCESS) {
            return result;
        }
    }
    
    // Extract frustum planes
    FrustumInternal frustum;
    GaussianResult result = extract_frustum_planes(view_proj_matrix, &frustum);
    if (result != GAUSSIAN_SUCCESS) {
        return result;
    }
    
    // Update frame counter
    g_current_frame++;
    g_visibility_history.frame_number = g_current_frame;
    
    u32 visible_count = 0;
    
    // First pass: Hierarchical culling - test cells against frustum
    for (int cell_idx = 0; cell_idx < TOTAL_GRID_CELLS; cell_idx++) {
        SpatialGridCell* cell = &g_spatial_grid.cells[cell_idx];
        
        if (cell->splat_count == 0) continue;
        
        // Test cell AABB against frustum
        bool cell_visible = aabb_intersects_frustum(cell->bounds_min, cell->bounds_max, &frustum);
        
        if (!cell_visible) {
            // Mark all splats in this cell as not visible
            for (u32 i = 0; i < cell->splat_count; i++) {
                update_visibility_history(cell->splat_indices[i], false);
            }
            cell->is_visible = false;
            continue;
        }
        
        cell->is_visible = true;
        cell->last_visible_frame = g_current_frame;
        
        // Second pass: Test individual splats in visible cells
        u32 batch_start = 0;
        while (batch_start < cell->splat_count) {
            u32 batch_size = (cell->splat_count - batch_start > VU0_BATCH_SIZE) ? 
                            VU0_BATCH_SIZE : (cell->splat_count - batch_start);
            
            bool batch_results[VU0_BATCH_SIZE];
            
            // Process batch with VU0 (or C fallback)
            vu0_cull_batch(input_splats, &cell->splat_indices[batch_start], batch_size, &frustum, batch_results);
            
            // Collect results
            for (u32 i = 0; i < batch_size; i++) {
                u32 splat_idx = cell->splat_indices[batch_start + i];
                bool is_visible = batch_results[i];
                
                // Apply temporal coherence optimization
                if (!is_visible && has_temporal_coherence(splat_idx)) {
                    // Skip culling test for splats with strong temporal coherence
                    is_visible = true;
                }
                
                update_visibility_history(splat_idx, is_visible);
                
                if (is_visible && visible_count < input_count) {
                    output_splats[visible_count] = input_splats[splat_idx];
                    visible_count++;
                }
            }
            
            batch_start += batch_size;
        }
    }
    
    *output_count = visible_count;
    return GAUSSIAN_SUCCESS;
}

// Get culling statistics
GaussianResult get_culling_stats(CullingStats* stats) {
    if (!stats) {
        return GAUSSIAN_ERROR_INVALID_PARAMETER;
    }
    
    stats->total_splats = g_spatial_grid.total_splats;
    stats->total_cells = TOTAL_GRID_CELLS;
    stats->visible_cells = 0;
    stats->empty_cells = 0;
    stats->frame_number = g_current_frame;
    
    for (int i = 0; i < TOTAL_GRID_CELLS; i++) {
        if (g_spatial_grid.cells[i].splat_count == 0) {
            stats->empty_cells++;
        } else if (g_spatial_grid.cells[i].is_visible) {
            stats->visible_cells++;
        }
    }
    
    return GAUSSIAN_SUCCESS;
}

// Test if a sphere is visible within the current frustum
bool is_sphere_visible(const fixed16_t center[3], fixed16_t radius, void* frustum_ptr) {
    if (!frustum_ptr) {
        return true;  // No frustum data, assume visible
    }
    
    const FrustumInternal* frustum = (const FrustumInternal*)frustum_ptr;
    return sphere_intersects_frustum(center, radius, frustum);
}

// Cleanup culling system
void cleanup_frustum_culling(void) {
    memset(&g_spatial_grid, 0, sizeof(g_spatial_grid));
    memset(&g_visibility_history, 0, sizeof(g_visibility_history));
    g_current_frame = 0;
}