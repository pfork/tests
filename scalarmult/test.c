#include "api.h"
#include "../rsp.h"
#include "irq.h"

void test(void) {
  disable_irqs();
  volatile unsigned char s[crypto_scalarmult_curve25519_BYTES];
  volatile unsigned char s1[crypto_scalarmult_curve25519_BYTES];
  volatile unsigned char p[crypto_scalarmult_curve25519_BYTES];
  volatile unsigned char p1[crypto_scalarmult_curve25519_BYTES];
  volatile unsigned char k[crypto_scalarmult_curve25519_BYTES];
  volatile unsigned char k1[crypto_scalarmult_curve25519_BYTES];
  //int crypto_scalarmult_curve25519(unsigned char *,const unsigned char *,const unsigned char *);
  int i;
  for(i=0;i<crypto_scalarmult_curve25519_BYTES;i++) {
    s[i]=0;
    s1[i]=0xff;
  }
  crypto_scalarmult_curve25519_base((unsigned char*) p,(unsigned char*) s);
  rsp_dump((unsigned char*) p, 32);
  crypto_scalarmult_curve25519_base((unsigned char*) p1,(unsigned char*) s1);
  rsp_dump((unsigned char*) p1, 32);
  crypto_scalarmult_curve25519((unsigned char*) k,(unsigned char*) s, (unsigned char*) p1);
  rsp_dump((unsigned char*) k, 32);
  crypto_scalarmult_curve25519((unsigned char*) k1,(unsigned char*) s1, (unsigned char*) p);
  rsp_dump((unsigned char*) k1, 32);
  rsp_detach();
  rsp_finish();
}
