/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "ff.h"
#include "sd.h"
#include "led.h"

PARTITION VolToPart[] = { {0, 1}, {0, 2}, {0, 3}, {0, 4} };

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
   if(SD_Init() == SD_OK) return 0;
   return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
  // is initialized at boot-time
  return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
  if(SD_ReadMultiBlocks(buff, sector, 512, count) == SD_OK) {
    if(SD_WaitReadOperation() == SD_OK) {
      while(SD_GetStatus() != SD_TRANSFER_OK);
      reset_read_led;
      return RES_OK;
    }
  }
  reset_read_led;
  return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
  if(SD_WriteMultiBlocks((unsigned char*) buff, sector, 512, count) == SD_OK) {
    if(SD_WaitWriteOperation() == SD_OK) {
      while(SD_GetStatus() != SD_TRANSFER_OK);
      reset_write_led;
      return RES_OK;
    }
  }
  reset_write_led;
  return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
  if(cmd == CTRL_SYNC) {
    //return (SD_WaitWriteOperation() == SD_OK)?RES_OK:RES_ERROR;
    return RES_OK;
  }
  return RES_PARERR;
}
#endif

WCHAR ff_convert (WCHAR wch, UINT dir)
{
	if (wch < 0x80) {
		/* ASCII Char */
		return wch;
	}

	/* I don't support unicode it is too big! */
	return 0;
}

WCHAR ff_wtoupper (WCHAR wch) {
	if (wch < 0x80) {
		if (wch >= 'a' && wch <= 'z') {
			wch &= ~0x20;
		}
		return wch;
	}
	return 0;
}

DWORD get_fattime(void) {
  return 0;
}
