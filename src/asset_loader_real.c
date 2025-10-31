/*
 * SPLATSTORM X - Real Asset Loading System
 * Replaces stub functions with actual binary file loading
 * Based on your technical specifications for custom binary format
 */

#include <tamtypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "splatstorm_x.h"

// Binary file format header
typedef struct {
    u32 magic;          // 'SPLT' magic number
    u32 version;        // Format version
    u32 splat_count;    // Number of splats
    u32 reserved;       // Reserved for future use
} BinarySplatHeader;

#define SPLAT_MAGIC 0x53504C54  // 'SPLT'
#define SPLAT_VERSION 1

/*
 * Load Gaussian splat scene from binary file
 * Implements custom binary format for fast loading
 */
int load_gaussian_splat_scene(const char* filename, GaussianSplat3D** splats, int* count) {
    debug_log_info("Loading Gaussian splat scene: %s", filename);
    
    if (!filename || !splats || !count) {
        debug_log_error("Invalid parameters");
        return -1;
    }
    
    // Open file for reading
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        debug_log_error("Failed to open file: %s", filename);
        return -2;
    }
    
    // Read header
    BinarySplatHeader header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        debug_log_error("Failed to read file header");
        close(fd);
        return -3;
    }
    
    // Validate header
    if (header.magic != SPLAT_MAGIC) {
        debug_log_error("Invalid file magic: 0x%08x (expected 0x%08x)", 
                       header.magic, SPLAT_MAGIC);
        close(fd);
        return -4;
    }
    
    if (header.version != SPLAT_VERSION) {
        debug_log_error("Unsupported file version: %d (expected %d)", 
                       header.version, SPLAT_VERSION);
        close(fd);
        return -5;
    }
    
    if (header.splat_count == 0 || header.splat_count > 65536) {
        debug_log_error("Invalid splat count: %d", header.splat_count);
        close(fd);
        return -6;
    }
    
    debug_log_info("File header valid: %d splats, version %d", 
                   header.splat_count, header.version);
    
    // Allocate memory for splats (16-byte aligned for DMA)
    size_t splat_data_size = header.splat_count * sizeof(GaussianSplat3D);
    *splats = (GaussianSplat3D*)splatstorm_malloc(splat_data_size);
    if (!*splats) {
        debug_log_error("Failed to allocate memory for %d splats", header.splat_count);
        close(fd);
        return -7;
    }
    
    // Read splat data
    if (read(fd, *splats, splat_data_size) != (int)splat_data_size) {
        debug_log_error("Failed to read splat data");
        splatstorm_free(*splats);
        *splats = NULL;
        close(fd);
        return -8;
    }
    
    close(fd);
    *count = header.splat_count;
    
    debug_log_info("Successfully loaded %d splats from %s", *count, filename);
    return 0;
}

/*
 * Generate test splats for development and testing
 * Creates a simple test scene with known splat positions
 */
GaussianSplat3D* generate_test_splats(int count) {
    debug_log_info("Generating %d test splats", count);
    
    if (count <= 0 || count > 65536) {
        debug_log_error("Invalid test splat count: %d", count);
        return NULL;
    }
    
    // Allocate memory for test splats
    GaussianSplat3D* splats = (GaussianSplat3D*)splatstorm_malloc(count * sizeof(GaussianSplat3D));
    if (!splats) {
        debug_log_error("Failed to allocate memory for test splats");
        return NULL;
    }
    
    // Generate test splats in a grid pattern
    int grid_size = 1;
    while (grid_size * grid_size < count) grid_size++;
    
    for (int i = 0; i < count; i++) {
        GaussianSplat3D* splat = &splats[i];
        
        // Position in grid
        int x = i % grid_size;
        int y = i / grid_size;
        
        // Position (normalized coordinates, converted to fixed16)
        splat->pos[0] = FLOAT_TO_FIXED16((float)(x - grid_size/2) / (float)(grid_size/2));
        splat->pos[1] = FLOAT_TO_FIXED16((float)(y - grid_size/2) / (float)(grid_size/2));
        splat->pos[2] = 0;  // All splats at same depth
        
        // Color (rainbow pattern)
        float hue = (float)i / (float)count * 6.28f;  // 2*PI
        splat->color[0] = (u8)((sinf(hue) + 1.0f) * 127.5f);           // Red
        splat->color[1] = (u8)((sinf(hue + 2.09f) + 1.0f) * 127.5f);   // Green (2*PI/3)
        splat->color[2] = (u8)((sinf(hue + 4.19f) + 1.0f) * 127.5f);   // Blue (4*PI/3)
        
        // Covariance (identity matrix scaled down)
        splat->cov_exp = 7;  // Scale factor 2^(7-7) = 1.0
        for (int j = 0; j < 9; j++) {
            splat->cov_mant[j] = (j % 4 == 0) ? (fixed8_t)(0.05f * FIXED8_SCALE) : 0;  // Diagonal matrix
        }
        
        // Opacity (semi-transparent)
        splat->opacity = 204;  // 0.8 * 255
    }
    
    debug_log_info("Generated %d test splats in %dx%d grid", count, grid_size, grid_size);
    return splats;
}

/*
 * Save splats to binary file (for development tools)
 */
int save_gaussian_splat_scene(const char* filename, GaussianSplat3D* splats, int count) {
    debug_log_info("Saving %d splats to: %s", count, filename);
    
    if (!filename || !splats || count <= 0) {
        debug_log_error("Invalid parameters");
        return -1;
    }
    
    // Open file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        debug_log_error("Failed to create file: %s", filename);
        return -2;
    }
    
    // Write header
    BinarySplatHeader header;
    header.magic = SPLAT_MAGIC;
    header.version = SPLAT_VERSION;
    header.splat_count = count;
    header.reserved = 0;
    
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        debug_log_error("Failed to write file header");
        close(fd);
        return -3;
    }
    
    // Write splat data
    size_t splat_data_size = count * sizeof(GaussianSplat3D);
    if (write(fd, splats, splat_data_size) != (int)splat_data_size) {
        debug_log_error("Failed to write splat data");
        close(fd);
        return -4;
    }
    
    close(fd);
    debug_log_info("Successfully saved %d splats to %s", count, filename);
    return 0;
}

/*
 * Free splat memory
 */
void free_gaussian_splats(GaussianSplat3D* splats) {
    if (splats) {
        splatstorm_free(splats);
        debug_log_info("Freed splat memory");
    }
}

/*
 * Validate splat data integrity
 */
int validate_splat_data(GaussianSplat3D* splats, int count) {
    if (!splats || count <= 0) {
        debug_log_error("Invalid splat data");
        return -1;
    }
    
    int errors = 0;
    
    for (int i = 0; i < count; i++) {
        GaussianSplat3D* splat = &splats[i];
        
        // Check position (fixed16 values)
        for (int j = 0; j < 3; j++) {
            if (splat->pos[j] < (s32)FIXED16_MIN || splat->pos[j] > (s32)FIXED16_MAX) {
                debug_log_error("Invalid position in splat %d", i);
                errors++;
                break;
            }
        }
        
        // Check covariance matrix (fixed8_t is s16, check for reasonable values)
        for (int j = 0; j < 9; j++) {
            // Check for extreme values that might indicate corruption
            if (splat->cov_mant[j] == (s16)0x8000 || splat->cov_mant[j] == (s16)0x7FFF) {
                debug_log_warning("Extreme covariance value in splat %d: %d", i, splat->cov_mant[j]);
            }
        }
        
        // Check opacity (u8 value - no need to check > 255 since u8 max is 255)
        // Just validate it's not uninitialized (could check for reasonable range)
        if (splat->opacity == 0) {
            debug_log_warning("Zero opacity in splat %d", i);
        }
        
        // Limit error reporting
        if (errors >= 10) {
            debug_log_error("Too many errors, stopping validation");
            break;
        }
    }
    
    if (errors == 0) {
        debug_log_info("Splat data validation passed for %d splats", count);
    } else {
        debug_log_error("Splat data validation failed with %d errors", errors);
    }
    
    return errors;
}