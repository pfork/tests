#include "newhope.h"
#include "poly.h"
#include "../../crypto/randombytes_salsa20_random.h"
#include "error_correction.h"
#include <string.h>
#include "../rsp.h"
#include "oled.h"

#include "sphincs/crypto_sign.h"
#include "sphincs/api.h"

#include "stm32f.h"
#include "delay.h"
#include "nrf24l01.h"

#define CHANNEL 81

int test_keys(){
  poly sk_a;
  u8 key_a[32], key_b[32];
  u8 senda[POLY_BYTES];
  u8 sendb[POLY_BYTES];
  u8 pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  u8 sig[CRYPTO_BYTES];

  oled_print(0,0, (char*) "doing pqkex", Font_8x8);

  //oled_print(0,0, (char*) "keypair", Font_8x8);
  randombytes_salsa20_random_buf(sk, CRYPTO_SECRETKEYBYTES);
  crypto_sign_public_key(pk, sk);
  //oled_print(0,0, (char*) "keypair ok", Font_8x8);

  //Alice generates a public key
  //oled_print(0,9, (char*) "newhope1", Font_8x8);
  newhope_keygen(senda, &sk_a);
  //oled_clear();
  //oled_print(0,0, (char*) "newhope1 ok", Font_8x8);

  // calculate sig and output
  //oled_print(0,9, (char*) "sign..", Font_8x8);
  crypto_sign(sk, senda, POLY_BYTES, sig);
  //oled_clear();
  //oled_print(0,0, (char*) "signed", Font_8x8);

  //oled_print(0,9, (char*) "verify", Font_8x8);
  if(crypto_sign_open(sig, senda, POLY_BYTES, pk) == -1) {
     rsp_dump((uint8_t*) "TERROR", 6);
     return 1;
  }
  //oled_clear();
  //oled_print(0,0, (char*) "verified", Font_8x8);

  //Bob derives a secret key and creates a response
  //oled_print(0,9, (char*) "sharedb", Font_8x8);
  newhope_sharedb(key_b, sendb, senda);
  //oled_clear();
  //oled_print(0,0, (char*) "sharedb ok", Font_8x8);

  //oled_print(0,9, (char*) "sign..", Font_8x8);
  crypto_sign(sk, sendb, POLY_BYTES, sig);
  //oled_clear();
  //oled_print(0,0, (char*) "signed", Font_8x8);

  //Alice uses Bobs response to get her secret key
  //oled_print(0,9, (char*) "verify", Font_8x8);
  if(crypto_sign_open(sig, sendb, POLY_BYTES, pk) == -1) {
     rsp_dump((uint8_t*) "ERROR4", 6);
     return 1;
  }
  //oled_clear();
  //oled_print(0,0, (char*) "verify ok", Font_8x8);
  //oled_print(0,9, (char*) "shareda", Font_8x8);
  newhope_shareda(key_a, &sk_a, sendb);
  //oled_clear();
  //oled_print(0,0, (char*) "shareda", Font_8x8);

  if(memcmp(key_a, key_b, 32)!=0) {
     rsp_dump((uint8_t*) "ERROR5", 6);
     return -1;
  }
  rsp_dump((uint8_t*) "yippie", 6);
  oled_clear();
  oled_print(0,0, (char*) "pqex OK", Font_8x8);

  return 0;
}


void dmacpy_init(void);

void test(void){
  //rsp_detach();

  // rotate screen 180
  oled_cmd(0xa1);
  oled_cmd(0xc8);
  oled_clear();

  //uint8_t msg[32]={0x40,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  //uint8_t msg[32]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  //                 0x30,0x30,0x40,0x40,0x50,0x50,0x35,0x35,
  //                 0x20,0x20,0x55,0x45,0x35,0x25,0x20,0x53,
  //                 0x43,0x33,0x33,0x43,0x50,0x51,0x52,0x55};
  //u32 msglen = 32;
  uint8_t msg[1024]; uint32_t msglen=1024;

  nrf24_init();
  dmacpy_init();
  while(1) {
    u32 len=0;
    oled_clear();
    //oled_print(0,0, (char*) "sending", Font_8x8);
    ////if( (len = send(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", msg, msglen)) >0) {
    ////rsp_detach();
    //if( (len = send(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", (uint8_t*) 0x20000000, 1<<10)) >0) {
    //  oled_print(0,9, (char*) "sent   ", Font_8x8);
    //  rsp_dump((u8*) &len, 4);
    //} else {
    //  oled_print(0,9, (char*) "timeout", Font_8x8);
    //}
    oled_print(0,0, (char*) "recving", Font_8x8);
    if( (len = listen(CHANNEL, (unsigned char*) "\x1\x2\x3\x5\x5", msg, msglen)) >0) {
      oled_print(0,9, (char*) "recv   ", Font_8x8);
      mDelay(100);
      rsp_dump((u8*) &len, 4);
    } else {
      oled_print(0,9, (char*) "timeout", Font_8x8);
    }
    rsp_detach();
  }

  rsp_finish();
  test_keys();
}
