#include "user_diskio.h"
#include "logger.h"
#include "stm32h5xx_hal.h"

#define _USE_WRITE 1
#define _USE_IOCTL 1

extern SD_HandleTypeDef hsd1;
extern Disk_drvTypeDef disk;
static volatile DSTATUS Stat = STA_NOINIT;

#define SD_IO_TIMEOUT_MS 250U

DSTATUS USER_initialize(BYTE pdrv);
DSTATUS USER_status(BYTE pdrv);
DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#endif

Diskio_drvTypeDef USER_Driver = {
    USER_initialize, USER_status, USER_read,
#if _USE_WRITE == 1
    USER_write,
#endif
#if _USE_IOCTL == 1
    USER_ioctl,
#endif
};

DSTATUS USER_initialize(BYTE pdrv) {
  (void)pdrv;
  Stat = STA_NOINIT;
  HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
  HAL_SD_DeInit(&hsd1);
  HAL_Delay(2);
  HAL_StatusTypeDef st = HAL_SD_Init(&hsd1);
  LOG_INFO("SD init: st=%d err=0x%lx state=%d", (int)st,
           (unsigned long)HAL_SD_GetError(&hsd1), (int)hsd1.State);
  if (st == HAL_OK) {
    Stat &= ~STA_NOINIT;
  }
  return Stat;
}

DSTATUS USER_status(BYTE pdrv) { return Stat; }

void USER_diskio_invalidate(void) {
  Stat = STA_NOINIT;
  for (int i = 0; i < FF_VOLUMES; i++) {
    disk.is_initialized[i] = 0;
  }
}

DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  if (HAL_SD_ReadBlocks(&hsd1, buff, sector, count, SD_IO_TIMEOUT_MS) ==
      HAL_OK) {
    uint32_t timeout = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {
      if ((HAL_GetTick() - timeout) > SD_IO_TIMEOUT_MS) {
        return RES_ERROR;
      }
    }
    return RES_OK;
  }
  LOG_ERROR("SD read fail: sector=%lu err=0x%lx", (unsigned long)sector,
            (unsigned long)HAL_SD_GetError(&hsd1));
  return RES_ERROR;
}

#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *)buff, sector, count,
                         SD_IO_TIMEOUT_MS) == HAL_OK) {
    uint32_t timeout = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {
      if ((HAL_GetTick() - timeout) > SD_IO_TIMEOUT_MS) {
        return RES_ERROR;
      }
    }
    return RES_OK;
  }
  return RES_ERROR;
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;

  switch (cmd) {
  case CTRL_SYNC:
    res = RES_OK;
    break;
  case GET_SECTOR_COUNT:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockNbr;
    res = RES_OK;
    break;
  case GET_SECTOR_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(WORD *)buff = CardInfo.LogBlockSize;
    res = RES_OK;
    break;
  case GET_BLOCK_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockSize / 512;
    res = RES_OK;
    break;
  default:
    res = RES_PARERR;
  }
  return res;
}
#endif