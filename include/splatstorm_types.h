/*
 * SPLATSTORM X - Core Type Definitions
 * Shared types to avoid circular dependencies
 */

#ifndef SPLATSTORM_TYPES_H
#define SPLATSTORM_TYPES_H

// Basic types
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef int s32;
typedef short s16;
typedef signed char s8;

// Core splat structure
typedef struct {
    float pos[3];       // Position (x, y, z)
    float color[4];     // RGBA color (includes alpha)
    float scale[2];     // Scale (width, height)
    float rotation;     // Rotation angle
} splat_t;

// Frustum structure (shared between headers)
typedef struct {
    float planes[6][4];  // 6 frustum planes (left, right, top, bottom, near, far)
} Frustum;

#endif // SPLATSTORM_TYPES_H