/*
 * SPLATSTORM X - Enhanced IOP Module System
 * Based on AthenaEnv's comprehensive module management
 * Supports 19+ IRX modules with dependency resolution
 */

#ifndef IOP_MODULES_H
#define IOP_MODULES_H

#include <sifrpc.h>
#include <loadfile.h>
#include <libmc.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <smod.h>
#include <sbv_patches.h>
#include <smem.h>
#include <libpwroff.h>
#include <stdbool.h>

// Module status flags
extern bool kbd_started;
extern bool mouse_started;
extern bool freeram_started;
extern bool ds34bt_started;
extern bool ds34usb_started;
extern bool network_started;
extern bool sio2man_started;
extern bool usbd_started;
extern bool usb_mass_started;
extern bool pads_started;
extern bool audio_started;
extern bool bdm_started;
extern bool mmceman_started;
extern bool cdfs_started;
extern bool dev9_started;
extern bool mc_started;
extern bool hdd_started;
extern bool filexio_started;
extern bool camera_started;
extern bool HDD_USABLE;

// IRX definition macro
#define irx_define(mod) \
    extern unsigned char mod##_irx[] __attribute__((aligned(16))); \
    extern unsigned int size_##mod##_irx

// Enhanced module list with dependency resolution
enum ENHANCED_MODLIST {
    USBD_MODULE = 0,
    KEYBOARD_MODULE,
    MOUSE_MODULE,
    FREERAM_MODULE,
    DS34BT_MODULE,      // DualShock 3/4 Bluetooth
    DS34USB_MODULE,     // DualShock 3/4 USB
    NETWORK_MODULE,
    USB_MASS_MODULE,
    PADS_MODULE,
    AUDIO_MODULE,
    MMCEMAN_MODULE,
    BDM_MODULE,
    CDFS_MODULE,
    MC_MODULE,
    HDD_MODULE,
    FILEXIO_MODULE,
    SIO2MAN_MODULE,
    DEV9_MODULE,
    CAMERA_MODULE,
    // Additional modules for enhanced functionality
    NETMAN_MODULE,
    PS2IP_MODULE,
    SMAP_MODULE,
};

#define BOOT_MODULE 99

// Core IRX modules
irx_define(iomanX);
irx_define(fileXio);
irx_define(sio2man);
irx_define(mcman);
irx_define(mcserv);
irx_define(padman);
irx_define(mtapman);
irx_define(mmceman);
irx_define(cdfs);
irx_define(usbd);
irx_define(bdm);
irx_define(bdmfs_fatfs);
irx_define(usbmass_bd);
irx_define(ps2dev9);
irx_define(ps2atad);
irx_define(ps2hdd);
irx_define(ps2fs);

// COMPLETE IMPLEMENTATION - Network modules always available
irx_define(SMAP);
irx_define(NETMAN);
irx_define(ps2ip);

// COMPLETE IMPLEMENTATION - Audio modules always available
irx_define(libsd);
irx_define(audsrv);

// COMPLETE IMPLEMENTATION - Input modules always available
irx_define(ps2kbd);
irx_define(ps2mouse);
irx_define(ps2cam);

// COMPLETE IMPLEMENTATION - Controller emulation modules always available
irx_define(ds34bt);
irx_define(ds34usb);

// System modules
irx_define(poweroff);
irx_define(freeram);

// Module dependency structure
typedef struct {
    int module_id;
    int* dependencies;
    int dep_count;
    const char* name;
    bool required;
} module_dependency_t;

// Function declarations
int get_boot_device(const char* path);
int load_enhanced_module(int id);
int load_module_with_dependencies(int id);
bool wait_device(char *path);
void prepare_IOP_enhanced(void);
int verify_module_loaded(int id);
void unload_all_modules(void);
int get_module_status(int id);

// Enhanced initialization functions
int iop_init_enhanced_modules(void);
int iop_load_network_stack(void);
int iop_load_audio_system(void);
int iop_load_input_devices(void);
int iop_load_storage_devices(void);

// Module management functions
void iop_print_module_status(void);
int iop_reload_module(int id);
int iop_get_module_memory_usage(int id);

#endif // IOP_MODULES_H
