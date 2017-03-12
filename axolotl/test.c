//#include "pitchfork.h"
#include "pf_store.h"
#include <string.h>
//#include "master.h"
//#include "usb.h"
#include "oled.h"
#include "xeddsa.h"
#include "xeddsa_keygen.h"
#include "axolotl.h"
#include "ramload/rsp.h"
#include "crypto_secretbox.h"
#include "crypto_generichash.h"

// out should reserve (crypto_secretbox_NONCEBYTES+PADDEDHCRYPTLEN-16) == 88 bytes
// mk should reserve crypto_secretbox_KEYBYTES == 32 bytes
void ax_send_headers(Axolotl_ctx *ctx, uint8_t *out, uint8_t *mk) {
  uint8_t *hnonce=out;
  int i,j;
  // check if we have a DHRs
  for(i=0,j=0;i<crypto_secretbox_KEYBYTES;i++) if(ctx->dhrs.sk[i]==0) j++;
  if(j==crypto_secretbox_KEYBYTES) { // if not, generate one, and reset counter
    randombytes_buf(ctx->dhrs.sk,crypto_scalarmult_curve25519_BYTES);
    crypto_scalarmult_curve25519_base(ctx->dhrs.pk, ctx->dhrs.sk);
    ctx->pns=ctx->ns;
    ctx->ns=0;
  }
  // derive message key
  crypto_generichash(mk, crypto_secretbox_KEYBYTES,       // output
                     ctx->cks, crypto_secretbox_KEYBYTES, // msg
                     (uint8_t*) "MK", 2);                 // "MK")
  // hnonce
  randombytes_buf(hnonce,crypto_secretbox_NONCEBYTES);

  // calculate Enc(HKs, Ns || PNs || DHRs)
  uint8_t header[PADDEDHCRYPTLEN]; // includes nacl padding
  memset(header,0,sizeof(header));
  // concat ns || pns || dhrs
  memcpy(header+32,&ctx->ns, sizeof(long long));
  memcpy(header+32+sizeof(long long),&ctx->pns, sizeof(long long));
  memcpy(header+32+sizeof(long long)*2, ctx->dhrs.pk, crypto_scalarmult_curve25519_BYTES);

  uint8_t header_enc[PADDEDHCRYPTLEN]; // also nacl padded
  // encrypt them
  crypto_secretbox(header_enc, header, sizeof(header), hnonce, ctx->hks);

  // unpad to output buf
  memcpy(hnonce+crypto_secretbox_NONCEBYTES, header_enc+16, sizeof(header_enc)-16);
  // send off headers
  ctx->ns++;
  crypto_generichash(ctx->cks, crypto_scalarmult_curve25519_BYTES, // output
                     ctx->cks, crypto_scalarmult_curve25519_BYTES, // msg
                     (uint8_t*) "CK", 2);                          // no key
  memset(header,0,sizeof(header));
}


void test(void) {
  // generate bobs lt private key and derive pubkey from it
  oled_clear();
  oled_print(0,0, "alice kp", Font_8x8);
  // alice lt private key and pubkey
  Axolotl_KeyPair kp;
  randombytes_buf(kp.sk, crypto_scalarmult_curve25519_BYTES);
  sc_clamp(kp.sk);
  curve25519_keygen(kp.pk,kp.sk);

  oled_print(0,0, "alice kp, pk", Font_8x8);
  // init alice prekey
  Axolotl_PreKey alice_pk;
  Axolotl_prekey_private alice_sk;
  memset(&alice_pk,0,sizeof(alice_pk));
  memset(&alice_sk,0,sizeof(alice_sk));
  axolotl_prekey(&alice_pk, &alice_sk, &kp, 1);

  oled_print(0,9, "bob kp", Font_8x8);
  randombytes_buf(kp.sk, crypto_scalarmult_curve25519_BYTES);
  sc_clamp(kp.sk);
  curve25519_keygen(kp.pk,kp.sk);

  oled_print(0,9, "bob kp, pk", Font_8x8);
  // generate bobs prekey
  Axolotl_PreKey bob_pk;
  Axolotl_prekey_private bob_sk;
  memset(&bob_pk,0,sizeof(bob_pk));
  memset(&bob_sk,0,sizeof(bob_sk));
  axolotl_prekey(&bob_pk, &bob_sk, &kp, 0);

  oled_print(0,9, "bob kp, pk, hs", Font_8x8);
  Axolotl_ctx bctx;
  if(axolotl_handshake(&bctx, &bob_pk, &alice_pk, &bob_sk)!=0) {
    oled_print_inv(0,32,"failed",Font_8x8);
    while(1);
  }

  oled_print(0,0, "alice kp, pk, hs", Font_8x8);
  Axolotl_ctx actx;
  if(axolotl_handshake(&actx, NULL, &bob_pk, &alice_sk)!=0) {
    // fail, try again
    oled_print_inv(0,32,"failed",Font_8x8);
    while(1);
  }

  oled_print(0,18, "bob sends", Font_8x8);
  uint8_t header[88], mk[32];
  ax_send_headers(&bctx, header, mk);

  uint8_t msg[]="                                Hello World!!!\n";
  uint8_t ctext[sizeof(msg)];
  int i;
  // zero out beginning of plaintext as demanded by nacl
  for(i=0;i<8;i++) ((unsigned int*) msg)[i]=0;
  // generate nonce
  uint8_t nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
  // encrypt into ctext
  crypto_secretbox(ctext, msg, sizeof(msg), nonce, mk);

  // we now have hnonce+headers in header, nonce and ctext
  // try to receive it

  oled_print(0,27, "alice recvs", Font_8x8);
  uint32_t out_len;
  uint8_t msg2[sizeof(msg)], mk2[32];
  if(0!=ax_recv(&actx, msg2, &out_len, header, nonce,
                header+crypto_secretbox_NONCEBYTES,
                ctext, sizeof(ctext), mk2)) {
    oled_print_inv(0,32,"failed",Font_8x8);
    while(1);
  }
  oled_print(0,27, "alice <3 bob", Font_8x8);

  //uint8_t buf[33];
  //uint8_t fname[]="/peers/6bc52af93ca16b6cebfbf8280c90a755";
  //cread (fname, buf, 0x20);

  //uint8_t fname[]="/testfile";
  //stfs_unlink(fname);
  //const uint8_t key[]="Hello World!";
  //write_enc(fname, key, sizeof(key));
  //cread(fname, buf, sizeof(buf));

  rsp_finish();
}
