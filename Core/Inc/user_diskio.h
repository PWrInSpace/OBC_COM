#ifndef __USER_DISKIO_H
#define __USER_DISKIO_H

#include "ff_gen_drv.h"

extern Diskio_drvTypeDef USER_Driver;

DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status     (BYTE pdrv);
DRESULT USER_read       (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);

#if _USE_WRITE == 1
DRESULT USER_write      (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl      (BYTE pdrv, BYTE cmd, void *buff);
#endif

#endif /* __USER_DISKIO_H */