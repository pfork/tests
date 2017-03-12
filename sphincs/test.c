#include "crypto_sign.h"
#include "api.h"
#include "../../rsp.h"
#include "oled.h"
#include "randombytes_pitchfork.h"

#define MLEN 32

void run(void) {
	unsigned char sphincs_sk[PQCRYPTO_SECRETKEYBYTES];
	unsigned char wrong[PQCRYPTO_SECRETKEYBYTES];
	unsigned char sphincs_pk[PQCRYPTO_PUBLICKEYBYTES];
   unsigned char sig[PQCRYPTO_BYTES];

   // rotate screen 180
   //oled_cmd(0xa1);
   //oled_cmd(0xc8);

   int i;
	for(i=0; i<PQCRYPTO_SECRETKEYBYTES; i++) sphincs_sk[i] = 12;
   randombytes_buf(sphincs_sk, PQCRYPTO_SECRETKEYBYTES);
   randombytes_buf(wrong, PQCRYPTO_SECRETKEYBYTES);

   oled_clear();
   oled_print(0,0, "key generate.", Font_8x8);
	pqcrypto_sign_public_key(sphincs_pk, sphincs_sk);
   oled_print(0,0, "key generated", Font_8x8);

   oled_print(0,9, "key sign..", Font_8x8);
	pqcrypto_sign(sphincs_sk, sphincs_sk, 32, sig);
   oled_clear();
   oled_print(0,0, "key generated", Font_8x8);
   oled_print(0,9, "key signed", Font_8x8);

   unsigned int x = (unsigned char)pqcrypto_sign_open(sig, sphincs_sk, 32, sphincs_pk);
   oled_clear();
   oled_print(0,0, "key generated", Font_8x8);
   oled_print(0,9, "key signed", Font_8x8);
   oled_print(0,18, "verify: ", Font_8x8);
   if(x == 0)
     oled_print(72,18, "OK", Font_8x8);
   else
     oled_print(72,18, "NOT OK", Font_8x8);
   oled_print(0,27, "verify2: ", Font_8x8);
   unsigned int y = (unsigned char)pqcrypto_sign_open(sig, wrong, 32, sphincs_pk);
   oled_clear();
   oled_print(0,0, "key generated", Font_8x8);
   oled_print(0,9, "key signed", Font_8x8);
   oled_print(0,18, "verify: ", Font_8x8);
   if(x == 0)
     oled_print(72,18, "OK", Font_8x8);
   else
     oled_print(72,18, "NOT OK", Font_8x8);
   oled_print(0,27, "verify2: ", Font_8x8);
   if(y == 0)
     oled_print(72,27, "OK", Font_8x8);
   else
     oled_print(72,27, "NOT OK", Font_8x8);
   rsp_dump((unsigned char*) &x,4);
   rsp_dump((unsigned char*) "jippie!",7);
   rsp_detach();
}

void test(void) { run(); while(1);}
