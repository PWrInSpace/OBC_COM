#include "user_diskio.h"
#include "stm32h5xx_hal.h"

static volatile DSTATUS Stat = STA_NOINIT;

Diskio_drvTypeDef USER_Driver = {
  USER_initialize,
  USER_status,
  USER_read,
#if _USE_WRITE == 1
  USER_write,
#endif
#if _USE_IOCTL == 1
  USER_ioctl,
#endif
};

DSTATUS USER_initialize(BYTE pdrv) {
  Stat = STA_NOINIT;
  
  // EXAMPLE FOR SDMMC:
  // if (HAL_SD_Init(&hsd1) == HAL_OK) { Stat &= ~STA_NOINIT; }
  
  Stat &= ~STA_NOINIT; 
  return Stat;
}

DSTATUS USER_status(BYTE pdrv) {
  return Stat;
}

DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  DRESULT res = RES_ERROR;

  // EXAMPLE:
  // if (HAL_SD_ReadBlocks(&hsd1, buff, sector, count, 1000) == HAL_OK) {
  //    res = RES_OK;
  // }
  
  res = RES_OK;
  return res;
}

#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  DRESULT res = RES_ERROR;

  // EXAMPLE:
  // if (HAL_SD_WriteBlocks(&hsd1, (BYTE*)buff, sector, count, 1000) == HAL_OK) {
  //    res = RES_OK;
  // }

  res = RES_OK;
  return res;
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
  DRESULT res = RES_ERROR;
  
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd) {
    case CTRL_SYNC:        res = RES_OK; break;
    case GET_SECTOR_COUNT: *(DWORD*)buff = 100000; res = RES_OK; break;
    case GET_SECTOR_SIZE:  *(WORD*)buff  = 512;    res = RES_OK; break;
    case GET_BLOCK_SIZE:   *(DWORD*)buff = 1;      res = RES_OK; break;
    default:               res = RES_PARERR;
  }
  return res;
}
#endif