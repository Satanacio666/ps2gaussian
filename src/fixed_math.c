/*
 * SPLATSTORM X - Fixed-Point Math Library
 * No floating point in critical paths - prevents NaN/overflow crashes
 */

#include "splatstorm_x.h"
#include "splatstorm_debug.h"
#include <string.h>

// Sine/Cosine lookup tables (16.16 fixed point)
static fixed16_t sin_table[1024];
static fixed16_t cos_table[1024];
static u32 math_initialized = 0;

// Square root lookup table for common values
static fixed16_t sqrt_table[256];

void fixed_math_init(void)
{
    fixed_math_init_tables();
}

void fixed_math_init_tables(void)
{
    if (math_initialized) return;
    
    debug_log_info("Initializing fixed-point math tables...");
    
    // Generate sine/cosine tables
    int i; for (i = 0; i < 1024; i++) {
        float angle = (float)i * (2.0f * 3.14159265f) / 1024.0f;
        sin_table[i] = FLOAT_TO_FIXED16(sinf(angle));
        cos_table[i] = FLOAT_TO_FIXED16(cosf(angle));
    }
    
    // Generate square root table
    int j; for (j = 0; j < 256; j++) {
        float value = (float)j / 256.0f * 16.0f;  // 0 to 16
        sqrt_table[j] = FLOAT_TO_FIXED16(sqrtf(value));
    }
    
    math_initialized = 1;
    debug_log_info("Fixed-point math initialized");
}

fixed16_t fixed16_mul(fixed16_t a, fixed16_t b)
{
    // 32x32 -> 64 bit multiply, then shift down
    s64 result = ((s64)a * (s64)b) >> 16;
    
    // Clamp to prevent overflow
    if (result > 0x7FFFFFFF) return 0x7FFFFFFF;
    if (result < -0x80000000) return -0x80000000;
    
    return (fixed16_t)result;
}

fixed16_t fixed16_div(fixed16_t a, fixed16_t b)
{
    if (b == 0) {
        debug_log_error("Division by zero in fixed16_div");
        return (a >= 0) ? 0x7FFFFFFF : -0x80000000;  // Return max/min instead of crashing
    }
    
    // Shift up then divide
    s64 result = ((s64)a << 16) / (s64)b;
    
    // Clamp to prevent overflow
    if (result > 0x7FFFFFFF) return 0x7FFFFFFF;
    if (result < -0x80000000) return -0x80000000;
    
    return (fixed16_t)result;
}

fixed16_t fixed16_sin(fixed16_t angle)
{
    // Normalize angle to 0-1023 range
    u32 index = ((u32)angle >> 6) & 1023;  // Divide by 64, mask to 1024 entries
    return sin_table[index];
}

fixed16_t fixed16_cos(fixed16_t angle)
{
    // Normalize angle to 0-1023 range
    u32 index = ((u32)angle >> 6) & 1023;  // Divide by 64, mask to 1024 entries
    return cos_table[index];
}

fixed16_t fixed16_sqrt(fixed16_t value)
{
    if (value <= 0) return 0;
    
    // For small values, use lookup table
    if (value < (16 << 16)) {
        u32 index = (u32)value >> 16;  // Get integer part
        if (index < 256) {
            return sqrt_table[index];
        }
    }
    
    // For larger values, use Newton-Raphson iteration
    fixed16_t x = value >> 1;  // Initial guess
    
    int i; for (i = 0; i < 8; i++) {  // 8 iterations should be enough
        if (x == 0) break;
        fixed16_t x_new = (x + fixed16_div(value, x)) >> 1;
        if (x_new == x) break;  // Converged
        x = x_new;
    }
    
    return x;
}

fixed16_t fixed16_abs(fixed16_t value)
{
    return (value < 0) ? -value : value;
}

fixed16_t fixed16_neg(fixed16_t value)
{
    return -value;
}

fixed16_t fixed16_min(fixed16_t a, fixed16_t b)
{
    return (a < b) ? a : b;
}

fixed16_t fixed16_max(fixed16_t a, fixed16_t b)
{
    return (a > b) ? a : b;
}

fixed16_t fixed16_clamp(fixed16_t value, fixed16_t min_val, fixed16_t max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Vector math with fixed point
void fixed16_vec3_add(fixed16_t* result, const fixed16_t* a, const fixed16_t* b)
{
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

void fixed16_vec3_sub(fixed16_t* result, const fixed16_t* a, const fixed16_t* b)
{
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

fixed16_t fixed16_vec3_dot(const fixed16_t* a, const fixed16_t* b)
{
    fixed16_t dot = fixed16_mul(a[0], b[0]);
    dot += fixed16_mul(a[1], b[1]);
    dot += fixed16_mul(a[2], b[2]);
    return dot;
}

fixed16_t fixed16_vec3_length(const fixed16_t* v)
{
    fixed16_t len_sq = fixed16_mul(v[0], v[0]);
    len_sq += fixed16_mul(v[1], v[1]);
    len_sq += fixed16_mul(v[2], v[2]);
    return fixed16_sqrt(len_sq);
}

void fixed16_vec3_normalize(fixed16_t* result, const fixed16_t* v)
{
    fixed16_t length = fixed16_vec3_length(v);
    if (length == 0) {
        result[0] = FIXED16_ONE;
        result[1] = 0;
        result[2] = 0;
        return;
    }
    
    result[0] = fixed16_div(v[0], length);
    result[1] = fixed16_div(v[1], length);
    result[2] = fixed16_div(v[2], length);
}

// Matrix operations (4x4 matrices stored as 16 fixed16_t values)
void fixed16_mat4_identity(fixed16_t* matrix)
{
    memset(matrix, 0, 16 * sizeof(fixed16_t));
    matrix[0] = FIXED16_ONE;   // m[0][0]
    matrix[5] = FIXED16_ONE;   // m[1][1]
    matrix[10] = FIXED16_ONE;  // m[2][2]
    matrix[15] = FIXED16_ONE;  // m[3][3]
}

void fixed16_mat4_multiply(fixed16_t* result, const fixed16_t* a, const fixed16_t* b)
{
    fixed16_t temp[16];
    
    int i; for (i = 0; i < 4; i++) {
        int j; for (j = 0; j < 4; j++) {
            temp[i*4 + j] = 0;
            int k; for (k = 0; k < 4; k++) {
                temp[i*4 + j] += fixed16_mul(a[i*4 + k], b[k*4 + j]);
            }
        }
    }
    
    memcpy(result, temp, 16 * sizeof(fixed16_t));
}

void fixed16_mat4_vec4_multiply(fixed16_t* result, const fixed16_t* matrix, const fixed16_t* vector)
{
    int i; for (i = 0; i < 4; i++) {
        result[i] = 0;
        int j; for (j = 0; j < 4; j++) {
            result[i] += fixed16_mul(matrix[i*4 + j], vector[j]);
        }
    }
}

// Interpolation functions
fixed16_t fixed16_lerp(fixed16_t a, fixed16_t b, fixed16_t t)
{
    // Clamp t to [0, 1]
    t = fixed16_clamp(t, 0, FIXED16_ONE);
    
    // a + t * (b - a)
    return a + fixed16_mul(t, b - a);
}

fixed16_t fixed16_smoothstep(fixed16_t edge0, fixed16_t edge1, fixed16_t x)
{
    // Clamp x to [edge0, edge1]
    x = fixed16_clamp(x, edge0, edge1);
    
    // Normalize to [0, 1]
    fixed16_t t = fixed16_div(x - edge0, edge1 - edge0);
    
    // Smooth interpolation: t * t * (3 - 2 * t)
    fixed16_t t2 = fixed16_mul(t, t);
    
    return fixed16_mul(t2, (3 * FIXED16_ONE) - fixed16_mul(2 * FIXED16_ONE, t));
}

// Individual conversion functions
fixed16_t float_to_fixed16(float value)
{
    return FLOAT_TO_FIXED16(value);
}

fixed8_t float_to_fixed8(float value)
{
    return FLOAT_TO_FIXED8(value);
}

// Utility functions for converting between float and fixed point arrays
void float_to_fixed16_array(fixed16_t* dest, const float* src, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++) {
        dest[i] = FLOAT_TO_FIXED16(src[i]);
    }
}

void fixed16_to_float_array(float* dest, const fixed16_t* src, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++) {
        dest[i] = FIXED16_TO_FLOAT(src[i]);
    }
}

// Debug functions
void fixed16_print(const char* name, fixed16_t value)
{
    float f_value = FIXED16_TO_FLOAT(value);
    debug_log_verbose("%s = %f (0x%08X)", name, f_value, value);
}

void fixed16_vec3_print(const char* name, const fixed16_t* vec)
{
    debug_log_verbose("%s = [%f, %f, %f]", name, 
        FIXED16_TO_FLOAT(vec[0]), 
        FIXED16_TO_FLOAT(vec[1]), 
        FIXED16_TO_FLOAT(vec[2]));
}