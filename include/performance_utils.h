/*
 * SPLATSTORM X - Performance Utilities
 * CPU cycle counting and timing functions for PS2
 */

#ifndef PERFORMANCE_UTILS_H
#define PERFORMANCE_UTILS_H

#include <tamtypes.h>

// CPU cycle counting using PS2 performance counter
// Implementation provided in performance_counters.c
extern u64 get_cpu_cycles(void);

// Convert CPU cycles to milliseconds (assuming 294.912 MHz EE clock)
// COMPLETE IMPLEMENTATION - Moved from static inline to performance_counters.c
extern float cycles_to_ms(u64 cycles);

// Convert CPU cycles to microseconds
// COMPLETE IMPLEMENTATION - Moved from static inline to performance_counters.c
extern float cycles_to_us(u64 cycles);

// Get high-resolution timer (microseconds since boot)
// COMPLETE IMPLEMENTATION - Moved from static inline to performance_counters.c
extern u64 get_timer_us(void);

// VU0 register reading functionality
// COMPLETE IMPLEMENTATION - Full VU0 register access
extern void read_vu0_register(u32 reg_num, float* result);
extern void read_all_vu0_registers(float registers[32][4]);

#endif // PERFORMANCE_UTILS_H