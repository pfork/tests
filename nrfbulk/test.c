#include "nrf.h"
#include "delay.h"
#include "oled.h"
#include <stdint.h>
#include <string.h>

static int send_buf(uint8_t *dst, uint8_t *buf, uint32_t size) {
  // try to send 1st pkt
  int i, retries;
  for(retries=0;retries<5;retries++) {
    if(nrf_send(dst,buf,size<=32?size:32)==0) {
      mDelay(10);
      continue;
    }
    // send the rest of the pkts
    int len;
    for(i=32;i<size;i+=len) {
      len = (size-i)>=32?32:(size-i);
      int retries1, ret=0;
      for(retries1=0;retries1<5 && ret==0;retries1++) ret=nrf_send(dst,buf+i,len);
      if(retries1>=5) break;
    }
    if(i==size) break;
    mDelay(10);
  }
  return retries<5;
}

static int recv_fast(uint8_t *buf, uint32_t size) {
  int i, len, retries, res;
  for(i=0;i<size;i+=len) {
    len = (size-i)>=32?32:(size-i);
    for(retries=0;retries<5;) {
      res=nrf_recv(buf+i, len);
      if((res & 0x80) == 1) continue; //broadcast
      if((res & 0x7f) == len)  break;
      retries++;
    }
    if(retries>=5) return 0;
  }
  return size;
}

static int recv_buf(uint8_t *src, uint8_t *buf, uint32_t size) {
  nrf_open_rx(src);
  int retries, res, len;
  for(retries=0;retries<5;) {
    len=size<=32?size:32;
    res=nrf_recv(buf, len);
    if((res & 0x80) == 1) continue; //broadcast
    if((res & 0x7f) == len) break;
    mDelay(15);
    retries++;
  }
  if(retries>=5) return 0;
  // try to receive rest of the packets
  return recv_fast(buf+32, size-32)==size-32;
}

volatile uint8_t sender;

void test(void) {
  uint8_t msg[3079];
  int i, status=0, done=0;
  oled_clear();
  if(sender) {
    while(!done) {
      switch(status) {
      case 0: { // initialize
        oled_print(0,0,"sender", Font_8x8);
        for(i=0;i<sizeof(msg);i++) msg[i]=i%256;
        status++;
      }
      case 1: { // send 1st message
        if(1==send_buf((uint8_t*) "abcd", msg, sizeof(msg))) {
          oled_print(0,9,"sent", Font_8x8);
          status++;
        } else break;
      }
      case 2: { // receive response
        if(1==recv_buf((uint8_t*) "abcd", msg, sizeof(msg))) {
          oled_print(0,18,"recv", Font_8x8);
          status++;
        } else break;
      }
      case 3: {
        done=1;
      }
    }
    }
  } else {
    while(!done) {
      switch(status) {
      case 0: { // initialize
        memset(msg,0,sizeof(msg));
        oled_print(0,0,"responder", Font_8x8);
        status++;
      }
      case 1: { // receive 1st message
        if(1==recv_buf((uint8_t*) "abcd", msg, sizeof(msg))) {
          oled_print(0,9,"received", Font_8x8);
          status++;
        } else break;
      }
      case 2: { // send response
        if(1==send_buf((uint8_t*) "abcd", msg, sizeof(msg))) {
          oled_print(0,18,"responded", Font_8x8);
          status++;
        } else break;
      }
      case 3: {
        done=1;
      }
    }
    }
  }
}
