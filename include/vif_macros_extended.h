/*
 * SPLATSTORM X - Extended VIF Macros
 * Missing VIF macros and functions for PS2 development
 * Based on PS2SDK vif_codes.h with additional functionality
 */

#ifndef VIF_MACROS_EXTENDED_H
#define VIF_MACROS_EXTENDED_H

#include <tamtypes.h>
#include <vif_codes.h>

// VIF macros - COMPLETE IMPLEMENTATION
// UNPACK_NUM and UNPACK_IMDT are provided by PS2SDK vif_codes.h (better implementation)

// VIF_SET_UNPACK: addr, num_qwords, vn_vl_format, flags  
#define VIF_SET_UNPACK(addr, num, format, flags) \
    VIF_CODE(UNPACK_IMDT(addr, 0, flags) | UNPACK_NUM(num), 0, (format), 0)

#define VIF_V4_32   0x0C  // Vector 4 elements, 32-bit each (V4-32)
#define VIF_V3_32   0x08  // Vector 3 elements, 32-bit each (V3-32)  
#define VIF_V2_32   0x04  // Vector 2 elements, 32-bit each (V2-32)
#define VIF_V1_32   0x00  // Vector 1 element, 32-bit (S-32)

#define VIF_V4_16   0x0D  // Vector 4 elements, 16-bit each (V4-16)
#define VIF_V3_16   0x09  // Vector 3 elements, 16-bit each (V3-16)
#define VIF_V2_16   0x05  // Vector 2 elements, 16-bit each (V2-16)
#define VIF_V1_16   0x01  // Vector 1 element, 16-bit (S-16)

#define VIF_V4_8    0x0E  // Vector 4 elements, 8-bit each (V4-8)
#define VIF_V3_8    0x0A  // Vector 3 elements, 8-bit each (V3-8)
#define VIF_V2_8    0x06  // Vector 2 elements, 8-bit each (V2-8)
#define VIF_V1_8    0x02  // Vector 1 element, 8-bit (S-8)

#define VIF_V4_5    0x0F  // Vector 4 elements, 5-bit each (V4-5)
#define VIF_V3_5    0x0B  // Vector 3 elements, 5-bit each (V3-5)
#define VIF_V2_5    0x07  // Vector 2 elements, 5-bit each (V2-5)
#define VIF_V1_5    0x03  // Vector 1 element, 5-bit (S-5)

// Extended VIF macros for easier use
#define VIF_SET_MSCAL(addr) \
    VIF_CODE(MSCAL_IMDT(addr), 0, VIF_CMD_MSCAL, 0)

#define VIF_SET_MSCALF(addr) \
    VIF_CODE(MSCALF_IMDT(addr), 0, VIF_CMD_MSCALF, 0)

#define VIF_SET_END() \
    VIF_CODE(0, 0, VIF_CMD_NOP, 0)

#define VIF_SET_FLUSH() \
    VIF_CODE(0, 0, VIF_CMD_FLUSH, 0)

#define VIF_SET_FLUSHE() \
    VIF_CODE(0, 0, VIF_CMD_FLUSHE, 0)

#define VIF_SET_FLUSHA() \
    VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0)

#define VIF_SET_STCYCL(cl, wl) \
    VIF_CODE(STCYCL_IMDT(cl, wl), 0, VIF_CMD_STCYCL, 0)

#define VIF_SET_OFFSET(offset) \
    VIF_CODE(OFFSET_IMDT(offset), 0, VIF_CMD_OFFSET, 0)

#define VIF_SET_BASE(base) \
    VIF_CODE(BASE_IMDT(base), 0, VIF_CMD_BASE, 0)

#define VIF_SET_ITOP(addr) \
    VIF_CODE(ITOP_IMDT(addr), 0, VIF_CMD_ITOP, 0)

#define VIF_SET_STMOD(mode) \
    VIF_CODE(STMOD_IMDT(mode), 0, VIF_CMD_STMOD, 0)

#define VIF_SET_MARK(mark) \
    VIF_CODE(MARK_IMDT(mark), 0, VIF_CMD_MARK, 0)

#define VIF_SET_DIRECT(size) \
    VIF_CODE(DIRECT_IMDT(size), 0, VIF_CMD_DIRECT, 0)

#define VIF_SET_DIRECTHL(addr) \
    VIF_CODE(DIRECTHL_IMDT(addr), 0, VIF_CMD_DIRECTHL, 0)

#define VIF_SET_MPG(loadaddr, size) \
    VIF_CODE(MPG_IMDT(loadaddr) | MPG_NUM(size), 0, VIF_CMD_MPG, 0)

// VIF packet building helpers
#define VIF_PACKET_START(packet) \
    do { \
        u32* _vif_ptr = (u32*)(packet);

#define VIF_PACKET_ADD(code) \
        *_vif_ptr++ = (code);

#define VIF_PACKET_END() \
    } while(0)

// VIF data transfer modes
#define VIF_UNPACK_S_32     0x00  // 32-bit
#define VIF_UNPACK_S_16     0x01  // 16-bit  
#define VIF_UNPACK_S_8      0x02  // 8-bit
#define VIF_UNPACK_V2_32    0x04  // 2x32-bit
#define VIF_UNPACK_V2_16    0x05  // 2x16-bit
#define VIF_UNPACK_V2_8     0x06  // 2x8-bit
#define VIF_UNPACK_V3_32    0x08  // 3x32-bit
#define VIF_UNPACK_V3_16    0x09  // 3x16-bit
#define VIF_UNPACK_V3_8     0x0A  // 3x8-bit
#define VIF_UNPACK_V4_32    0x0C  // 4x32-bit
#define VIF_UNPACK_V4_16    0x0D  // 4x16-bit
#define VIF_UNPACK_V4_8     0x0E  // 4x8-bit
#define VIF_UNPACK_V4_5     0x0F  // 4x5-bit

// VIF utility functions
static inline u32 vif_create_unpack_tag(u16 addr, u8 vn, u8 vl, u8 flg, u8 usn, u8 num) {
    return VIF_CODE(UNPACK_IMDT(addr, usn, flg) | UNPACK_NUM(num), 0, VIF_CMD_UNPACK(0, vn, vl), 0);
}

static inline u32 vif_create_stcycl_tag(u8 cl, u8 wl) {
    return VIF_CODE(STCYCL_IMDT(cl, wl), 0, VIF_CMD_STCYCL, 0);
}

static inline u32 vif_create_mscal_tag(u16 addr) {
    return VIF_CODE(MSCAL_IMDT(addr), 0, VIF_CMD_MSCAL, 0);
}

static inline u32 vif_create_flush_tag(void) {
    return VIF_CODE(0, 0, VIF_CMD_FLUSH, 0);
}

static inline u32 vif_create_direct_tag(u16 size) {
    return VIF_CODE(DIRECT_IMDT(size), 0, VIF_CMD_DIRECT, 0);
}

// VIF channel definitions
#define VIF0_CHANNEL    0
#define VIF1_CHANNEL    1

// VIF status flags
#define VIF_STAT_VPS_MASK   0x00000003
#define VIF_STAT_VEW        0x00000004
#define VIF_STAT_VGW        0x00000008
#define VIF_STAT_MRK        0x00000040
#define VIF_STAT_DBF        0x00000080
#define VIF_STAT_VSS        0x00000100
#define VIF_STAT_VFS        0x00000200
#define VIF_STAT_VIS        0x00000400
#define VIF_STAT_INT        0x00000800
#define VIF_STAT_ER0        0x00001000
#define VIF_STAT_ER1        0x00002000

// VIF error codes
#define VIF_ERR_NONE        0
#define VIF_ERR_TIMEOUT     1
#define VIF_ERR_INVALID     2
#define VIF_ERR_BUSY        3

// GIF flag constants - use PS2SDK definitions

#endif // VIF_MACROS_EXTENDED_H