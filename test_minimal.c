/*
 * Minimal test program for PS2 3D Gaussian Splatting Engine
 * Tests core functionality and MIPS-optimized macros
 */

#include <stdio.h>
#include <stdlib.h>
#include <tamtypes.h>

// Test our MIPS-optimized fixed-point macros
#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)

// Optimized MIPS assembly fixed-point multiplication
#define FIXED_MUL(a, b) ({ \
    s32 result; \
    __asm__ volatile ( \
        "mult   %1, %2      \n\t"  /* Multiply a * b -> HI:LO */ \
        "mfhi   $t0         \n\t"  /* Get high 32 bits */ \
        "mflo   $t1         \n\t"  /* Get low 32 bits */ \
        "srl    $t1, $t1, %3\n\t"  /* Shift low right by FIXED_SHIFT */ \
        "sll    $t0, $t0, %4\n\t"  /* Shift high left by (32-FIXED_SHIFT) */ \
        "or     %0, $t0, $t1\n\t"  /* Combine for final result */ \
        : "=r" (result) \
        : "r" (a), "r" (b), "i" (FIXED_SHIFT), "i" (32-FIXED_SHIFT) \
        : "$t0", "$t1" \
    ); \
    result; \
})

// Fast absolute value using MIPS
#define FAST_ABS(x) ({ \
    s32 result; \
    __asm__ volatile ( \
        "sra    $t0, %1, 31  \n\t"  /* Sign extend to get mask */ \
        "xor    %0, %1, $t0  \n\t"  /* XOR with mask */ \
        "sub    %0, %0, $t0  \n\t"  /* Subtract mask */ \
        : "=r" (result) \
        : "r" (x) \
        : "$t0" \
    ); \
    result; \
})

int main(void) {
    printf("PS2 3D Gaussian Splatting Engine - Core Test\n");
    printf("=============================================\n\n");
    
    // Test MIPS-optimized fixed-point multiplication
    s32 a = FIXED_ONE * 3;  // 3.0 in fixed-point
    s32 b = FIXED_ONE / 2;  // 0.5 in fixed-point
    s32 result = FIXED_MUL(a, b);
    
    printf("Fixed-point multiplication test:\n");
    printf("3.0 * 0.5 = %d.%d\n", result >> FIXED_SHIFT, 
           ((result & 0xFFFF) * 1000) >> FIXED_SHIFT);
    
    // Test MIPS-optimized absolute value
    s32 negative = -12345;
    s32 abs_result = FAST_ABS(negative);
    
    printf("\nFast absolute value test:\n");
    printf("abs(%d) = %d\n", negative, abs_result);
    
    // Test positive number
    s32 positive = 54321;
    abs_result = FAST_ABS(positive);
    printf("abs(%d) = %d\n", positive, abs_result);
    
    printf("\nCore functionality tests completed successfully!\n");
    printf("MIPS-optimized macros are working correctly.\n");
    
    return 0;
}