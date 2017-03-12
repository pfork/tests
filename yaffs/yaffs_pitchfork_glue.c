#include <stdlib.h>
#include <stdint.h>
#include "yaffs_guts.h"
#include "direct/yaffscfg.h"
#include "yaffs_packedtags2.h"
//#include "storage.h"
#include "irq.h"
#include "libopencm3/stm32/flash.h"

#define BYTESPERCHUNK 96
#define CHUNKSPERBLOCK 1024
#define SPAREBYTESPERCHUNK 16
#define STARTBLOCK 7 // should be 6 in production, only 7 to keep old fs running while developing
#define ENDBLOCK 11
#define NBLOCKS (ENDBLOCK - STARTBLOCK + 1)

typedef struct {
  unsigned char data[BYTESPERCHUNK];
  unsigned char oob[SPAREBYTESPERCHUNK];
} _Chunk;

typedef struct {
  _Chunk chunks[CHUNKSPERBLOCK];
} _Block;

typedef _Block _Blocks[NBLOCKS];

_Blocks *blocksptr = (_Blocks*) FLASH_SECTOR07; // should be sector 6 in production

void vPortFree( void *pv );
void *pvPortMalloc( size_t xWantedSize );

unsigned yaffs_trace_mask = 0x0; /* Disable logging */
static int yaffs_errno;

/*
 * yaffsfs_CheckMemRegion()
 * Check that access to an address is valid.
 * This can check memory is in bounds and is writable etc.
 *
 * Returns 0 if ok, negative if not.
 */
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	(void) size;
	(void) write_request;

	if(!addr)
		return -1;
   //if(addr<(void*) blocksptr || addr>=(void*) blocksptr+(128*1024*NBLOCKS))
   //  return -1;
   //if(addr+size<(void*) blocksptr || addr+size>=(void*) blocksptr+(128*1024*NBLOCKS))
   //  return -1;
	return 0;
}

/*
 * yaffs_bug_fn()
 * Function to report a bug.
 */

void yaffs_bug_fn(const char *file_name, int line_no) {
  (void)file_name;
  (void)line_no;
  while(1);
}

void yaffsfs_SetError(int err) {
	yaffs_errno = err;
}

int yaffsfs_GetLastError(void) {
	return yaffs_errno;
}

int yaffsfs_GetError(void) {
	return yaffs_errno;
}

void yaffsfs_Lock(void) {}

void yaffsfs_Unlock(void) {}

u32 yaffsfs_CurrentTime(void) {
	return 0;
}

void *yaffsfs_malloc(size_t size) {
	return pvPortMalloc(size);
}

void yaffsfs_free(void *ptr) {
	vPortFree(ptr);
}

void yaffsfs_OSInitialisation(void) {}

static int yflash2_WriteChunk (struct yaffs_dev *dev, int nand_chunk,
                               const u8 *data, int data_len,
                               const u8 *oob, int oob_len) {
  int retval = YAFFS_OK;
  int blk;
  int pg;
  u8 *x;

  blk = nand_chunk/CHUNKSPERBLOCK;
  pg = nand_chunk%CHUNKSPERBLOCK;

  disable_irqs();
  flash_unlock();
  if(data && data_len) {
    if(data_len<0 || data_len>BYTESPERCHUNK) {
      retval = YAFFS_FAIL;
      goto exit;
    }
    x = blocksptr[blk]->chunks[pg].data;
    flash_program((int) x, (u8*) data, data_len);
  }

  if(oob && oob_len) {
    if (oob_len<0 || oob_len>SPAREBYTESPERCHUNK) {
      retval = YAFFS_FAIL;
      goto exit;
    }
    x = &blocksptr[blk]->chunks[pg].data[BYTESPERCHUNK];
    flash_program((int) x, (u8*) oob, oob_len);
  }

 exit:
  flash_lock();
  enable_irqs();

  return retval;
}

static int yflash2_ReadChunk(struct yaffs_dev *dev, int nand_chunk,
				   u8 *data, int data_len,
				   u8 *oob, int oob_len,
				   enum yaffs_ecc_result *ecc_result) {
  int blk;
  int pg;

  blk = nand_chunk/CHUNKSPERBLOCK;
  pg = nand_chunk%CHUNKSPERBLOCK;

  if(data && data_len) {
    if(data_len<0 || data_len>BYTESPERCHUNK) {
      return YAFFS_FAIL;
    }
    memcpy(data,blocksptr[blk]->chunks[pg].data,BYTESPERCHUNK);
  }

  if(oob && oob_len) {
    if (oob_len<0 || oob_len>SPAREBYTESPERCHUNK) {
      return YAFFS_FAIL;
    }
    memcpy(oob,&blocksptr[blk]->chunks[pg].data[BYTESPERCHUNK],oob_len);
  }

  if (ecc_result)
    *ecc_result = YAFFS_ECC_RESULT_NO_ERROR;

  return YAFFS_OK;
}

static int yflash2_EraseBlock(struct yaffs_dev *dev, int block_no) {
  (void) dev;
  if(block_no < STARTBLOCK || block_no > ENDBLOCK) {
    return YAFFS_FAIL;
  }
  clear_flash(block_no);
  return YAFFS_OK;
}

static int yflash2_MarkBad(struct yaffs_dev *dev, int block_no) {
	u8 *x;
	(void) dev;

	x = &blocksptr[block_no]->chunks[0].data[BYTESPERCHUNK];
	memset(x,0,sizeof(struct yaffs_packed_tags2));

	return YAFFS_OK;
}

static int yflash2_CheckBad(struct yaffs_dev *dev, int block_no) {
	(void) dev;
	(void) block_no;
	//return YAFFS_OK;

	struct yaffs_ext_tags tags;
	int chunkNo;

	chunkNo = block_no * dev->param.chunks_per_block;

   u8 oob[SPAREBYTESPERCHUNK];

	yflash2_ReadChunk(dev,chunkNo,NULL,0,oob,SPAREBYTESPERCHUNK,0);
   yaffs_unpack_tags2(dev, &tags, (struct yaffs_packed_tags2 *) oob, !dev->param.no_tags_ecc);
	if(tags.block_bad) {
		return YAFFS_BLOCK_STATE_DEAD;
	}
	return YAFFS_OK;
}

static int yflash2_Initialise(struct yaffs_dev *dev) {
	(void) dev;
	return YAFFS_OK;
}

// int yaffs_StartUp(void);
//
// Return value: YAFFS_OK or YAFFS_FAIL
//
// This functions should be used to provide yaffs with many details
// about the properties of the underlying nand device. Within this
// function you will need to set up a ‘yaffs_Device’ (yaffs_guts.h)
// and a ‘yaffsfs_DeviceConfiguration’ (direct/yaffscfg.h). How to set
// up a yaffs device will follow in the next section. Once you have
// set up the required yaffs_Device structures you should call
// yaffs_initialise (yaffs_guts.h) with a pointer to a null terminated
// array of yaffs_DeviceConfiguration structures. A
// yaffs_DeviceConfiguration structure contains a pointer to a
// yaffs_Device and the ‘mount point’ string (prefix) that that device
// corresponds to.

static struct yaffs_dev _dev;

struct yaffs_dev* yaffs_StartUp(void) {
   struct yaffs_param *param;
   struct yaffs_driver *drv;
   struct yaffs_dev *dev=&_dev;

   memset(dev, 0, sizeof(*dev));

   dev->param.name = "/";

   drv = &dev->drv;

   drv->drv_write_chunk_fn = yflash2_WriteChunk;
   drv->drv_read_chunk_fn = yflash2_ReadChunk;
   drv->drv_erase_fn = yflash2_EraseBlock;
   drv->drv_mark_bad_fn = yflash2_MarkBad;
   drv->drv_check_bad_fn = yflash2_CheckBad;
   drv->drv_initialise_fn = yflash2_Initialise;

   param = &dev->param;

   param->total_bytes_per_chunk = BYTESPERCHUNK+SPAREBYTESPERCHUNK; // for inband tags add sparebytes
   param->chunks_per_block = CHUNKSPERBLOCK;
   param->spare_bytes_per_chunk=0; //SPAREBYTESPERCHUNK; inband
   param->inband_tags=1;
   param->max_objects=2048;
   param->start_block = STARTBLOCK;
   param->end_block = ENDBLOCK;
   param->is_yaffs2 = 1;
   param->no_tags_ecc=1;

   param->n_reserved_blocks = 2;
   param->n_caches = 0; // Use caches

   yaffs_add_device(dev);

   return dev;
}
