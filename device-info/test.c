#include <stdint.h>
#include "stm32f.h"
#include "delay.h"
#include "../rsp.h"

void test(void) {
  uint32_t *udid = (uint32_t*) 0x1fff7a10; // Unique device ID register uin32_t[3]
  uint16_t *flash_size = (uint16_t*) 0x1fff7a22; // flash size
  rsp_dump((uint8_t*)udid,12);
  rsp_dump((uint8_t*)flash_size,2);
  //rsp_detach();
  rsp_finish();
}
