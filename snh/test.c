#include "newhope.h"
#include "poly.h"
#include "../../crypto/randombytes_salsa20_random.h"
#include "error_correction.h"
#include <string.h>
#include "../rsp.h"
#include "oled.h"

#include "sphincs/crypto_sign.h"
#include "sphincs/api.h"

int test_keys(){
  poly sk_a;
  u8 key_a[32], key_b[32];
  u8 senda[POLY_BYTES];
  u8 sendb[POLY_BYTES];
  u8 pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  u8 sig[CRYPTO_BYTES];

  // rotate screen 180
  oled_cmd(0xa1);
  oled_cmd(0xc8);
  oled_clear();
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

void test(void){
  test_keys();
  rsp_detach();
  rsp_finish();
}
