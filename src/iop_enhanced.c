/*
 * SPLATSTORM X - IRX Module Loading System
 * Robust implementation with PS2SDK IRX modules embedded as C arrays
 * Follows proper loading order and error handling
 */

#include <tamtypes.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <stdio.h>
#include "splatstorm_x.h"

// External IRX module declarations (embedded as C arrays)
extern unsigned char iomanX_irx[];
extern unsigned int size_iomanX_irx;
extern unsigned char fileXio_irx[];
extern unsigned int size_fileXio_irx;
extern unsigned char ps2dev9_irx[];
extern unsigned int size_ps2dev9_irx;
extern unsigned char ps2fs_irx[];
extern unsigned int size_ps2fs_irx;
extern unsigned char usbmass_bd_irx[];
extern unsigned int size_usbmass_bd_irx;
extern unsigned char ps2hdd_irx[];
extern unsigned int size_ps2hdd_irx;
extern unsigned char ps2atad_irx[];
extern unsigned int size_ps2atad_irx;
extern unsigned char bdm_irx[];
extern unsigned int size_bdm_irx;
extern unsigned char bdmfs_fatfs_irx[];
extern unsigned int size_bdmfs_fatfs_irx;
extern unsigned char freeram_irx[];
extern unsigned int size_freeram_irx;

// Module loading status
static bool iop_modules_loaded = false;

/*
 * Load IRX modules in proper dependency order
 * Returns SPLATSTORM_OK on success, error code on failure
 */
int load_irx_modules(void) {
    int ret;
    
    if (iop_modules_loaded) {
        printf("[IOP] Modules already loaded\n");
        return SPLATSTORM_OK;
    }
    
    printf("[IOP] Initializing IOP heap and RPC...\n");
    SifInitIopHeap();
    SifInitRpc(0);
    
    // Load modules in dependency order
    printf("[IOP] Loading iomanX.irx (%u bytes)...\n", size_iomanX_irx);
    ret = SifExecModuleBuffer(iomanX_irx, size_iomanX_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load iomanX.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading ps2dev9.irx (%u bytes)...\n", size_ps2dev9_irx);
    ret = SifExecModuleBuffer(ps2dev9_irx, size_ps2dev9_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load ps2dev9.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading ps2atad.irx (%u bytes)...\n", size_ps2atad_irx);
    ret = SifExecModuleBuffer(ps2atad_irx, size_ps2atad_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load ps2atad.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading ps2hdd.irx (%u bytes)...\n", size_ps2hdd_irx);
    ret = SifExecModuleBuffer(ps2hdd_irx, size_ps2hdd_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load ps2hdd.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading ps2fs.irx (%u bytes)...\n", size_ps2fs_irx);
    ret = SifExecModuleBuffer(ps2fs_irx, size_ps2fs_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load ps2fs.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading fileXio.irx (%u bytes)...\n", size_fileXio_irx);
    ret = SifExecModuleBuffer(fileXio_irx, size_fileXio_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load fileXio.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading bdm.irx (%u bytes)...\n", size_bdm_irx);
    ret = SifExecModuleBuffer(bdm_irx, size_bdm_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load bdm.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading bdmfs_fatfs.irx (%u bytes)...\n", size_bdmfs_fatfs_irx);
    ret = SifExecModuleBuffer(bdmfs_fatfs_irx, size_bdmfs_fatfs_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load bdmfs_fatfs.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    printf("[IOP] Loading usbmass_bd.irx (%u bytes)...\n", size_usbmass_bd_irx);
    ret = SifExecModuleBuffer(usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP ERROR] Failed to load usbmass_bd.irx: %d\n", ret);
        return SPLATSTORM_ERROR_INIT;
    }
    
    // freeram.irx is optional - don't fail if it doesn't load
    printf("[IOP] Loading freeram.irx (%u bytes)...\n", size_freeram_irx);
    ret = SifExecModuleBuffer(freeram_irx, size_freeram_irx, 0, NULL, NULL);
    if (ret < 0) {
        printf("[IOP WARNING] freeram.irx not critical: %d\n", ret);
    }
    
    iop_modules_loaded = true;
    printf("[IOP] All IRX modules loaded successfully\n");
    return SPLATSTORM_OK;
}

/*
 * Initialize IOP system
 */
int iop_init(void) {
    printf("[IOP] Initializing IOP system...\n");
    
    // Reset IOP to clean state
    SifIopReset(NULL, 0);
    while (!SifIopSync()) {
        // Wait for IOP reset to complete
    }
    
    // Load all required IRX modules
    int ret = load_irx_modules();
    if (ret != SPLATSTORM_OK) {
        printf("[IOP ERROR] Failed to load IRX modules: %d\n", ret);
        return ret;
    }
    
    printf("[IOP] IOP system initialized successfully\n");
    return SPLATSTORM_OK;
}

/*
 * Shutdown IOP system
 */
void iop_shutdown(void) {
    printf("[IOP] Shutting down IOP system...\n");
    iop_modules_loaded = false;
}

/*
 * Check if IOP modules are loaded
 */
bool iop_modules_ready(void) {
    return iop_modules_loaded;
}