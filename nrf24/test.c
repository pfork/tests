#include <libopencm3/usb/usbd.h>
#include "stm32f.h"
#include "nrf24l01.h"
#include "pitchfork.h"
#include <string.h>
#include <delay.h>
#include <led.h>
#include <oled.h>
#include "../rsp.h"

#define CHANNEL 81

extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];

#define TO_HEX(i) (i <= 9 ? '0' + i : 'A' - 10 + i)
void tohex(uint32_t num, uint8_t* str) {
    str[0] = TO_HEX(((num & 0xF0000000) >> 28));
    str[1] = TO_HEX(((num & 0x0F000000) >> 24));
    str[2] = TO_HEX(((num & 0x00F00000) >> 20));
    str[3] = TO_HEX(((num & 0x000F0000) >> 16));
    str[4] = TO_HEX(((num & 0x0000F000) >> 12));
    str[5] = TO_HEX(((num & 0x00000F00) >> 8));
    str[6] = TO_HEX(((num & 0x000000F0) >> 4));
    str[7] = TO_HEX((num & 0x0000000F));
    str[8] = '\0';
}

volatile int i=0, last=0;
uint8_t msgs[7][32];

#include "dma.h"

void listen() {
//static void listen() {
  uint8_t tmp[9];
  if(nrf_recv(msgs[i], PLOAD_WIDTH)>0) {
    //rsp_dump(outbuf, len);
    if(i<7) i++;
    else {
      oled_clear();
      for(i=0;i<7;i++) {
        if(msgs[i][8]==0) {
          oled_print(0,i*8, (char*) msgs[i], Font_8x8);
        } else {
          tohex(*((uint32_t*) msgs[i]), tmp);
          oled_print(56,i*8, (char*) tmp, Font_8x8);
        }
      }
      i=0;
      mDelay(300);
    }
  }
}

void listen1() {
//static void listen() {
  int len = 0, cur, same = 0;
  uint8_t tmp[9];
  //nrf_recv(outbuf, PLOAD_WIDTH);
  if((len=nrf_recv(outbuf, PLOAD_WIDTH))>0) {
    oled_clear();
    if(outbuf[8]==0) {
        oled_print(0,54, (char*) outbuf, Font_8x8);
        cur = *((int*) outbuf+4);
        if(cur - last == 0) {
          same++;
        } else {
          tohex(same,tmp);
          oled_print(0,37, (char*) tmp, Font_8x8);
          same=0;
        }
        if(cur - last > 1) {
          tohex(cur - last,tmp);
          oled_print(0,46, (char*) tmp, Font_8x8);
        } else {
          oled_print(0,46, "             ", Font_8x8);
        }
        last = cur;
    } else {
      tohex(*((uint32_t*) outbuf), tmp);
      oled_print(0,54, (char*) tmp, Font_8x8);
    }
    rsp_dump(outbuf, len);
    if(i<20) i++;
  }
  if(i==20) {
    rsp_detach();
    i=21;
  }
}

void test(void) {
  //uint8_t msg[32]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  //                 0x0,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  //                 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  //                 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
  //uint32_t m=0;
  //nrf24_init();
  oled_clear();
  //unsigned char status=0;
  //status = nrf_read_reg(STATUS);
  //rsp_dump((uint8_t*) &status,1);
  //rsp_detach();
  i=0;
  rsp_detach();
  //rsp_dump((uint8_t*) "transmitting", 12);
  rsp_dump((uint8_t*) "Listening", 9);
  while(1) {
    //receive
    nrf_open(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", 1);
    listen();

    // send
    //rsp_dump((uint8_t*) "sending", 7);
    //nrf_open(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", 0);
    //tohex(m++,msg);
    //*((int*) msg+4) = m;
    //rsp_dump((uint8_t*) msg, 32);
    //nrf_send(msg,32);
    //nrf_send((unsigned char*) "it worksA!!5!\x0                  ",32);
    //rsp_detach();
  }
  rsp_finish();
}
