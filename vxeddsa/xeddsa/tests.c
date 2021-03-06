#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crypto_hash_sha512.h"
#include "keygen.h"
#include "curve_sigs.h"
#include "xeddsa.h"
#include "vxeddsa.h"
#include "crypto_additions.h"
#include "ge.h"
#include "utility.h"
#include <assert.h>


#define ERROR(...) do {if (!silent) { printf(__VA_ARGS__); abort(); } else return -1; } while (0)
#define INFO(...) do {if (!silent) printf(__VA_ARGS__);} while (0)

#define TEST(msg, cond) \
  do {  \
    if ((cond)) { \
      INFO("%s good\n", msg); \
    } \
    else { \
      ERROR("%s BAD!!!\n", msg); \
    } \
  } while (0)


int elligator_fast_test(int silent)
{
  unsigned char elligator_correct_output[32] = 
  {
  0x5f, 0x35, 0x20, 0x00, 0x1c, 0x6c, 0x99, 0x36, 
  0xa3, 0x12, 0x06, 0xaf, 0xe7, 0xc7, 0xac, 0x22, 
  0x4e, 0x88, 0x61, 0x61, 0x9b, 0xf9, 0x88, 0x72, 
  0x44, 0x49, 0x15, 0x89, 0x9d, 0x95, 0xf4, 0x6e
  };

  unsigned char hashtopoint_correct_output1[32] = 
  {
  0xce, 0x89, 0x9f, 0xb2, 0x8f, 0xf7, 0x20, 0x91,
  0x5e, 0x14, 0xf5, 0xb7, 0x99, 0x08, 0xab, 0x17,
  0xaa, 0x2e, 0xe2, 0x45, 0xb4, 0xfc, 0x2b, 0xf6,
  0x06, 0x36, 0x29, 0x40, 0xed, 0x7d, 0xe7, 0xed
  };

  unsigned char hashtopoint_correct_output2[32] = 
  {
  0xa0, 0x35, 0xbb, 0xa9, 0x4d, 0x30, 0x55, 0x33, 
  0x0d, 0xce, 0xc2, 0x7f, 0x83, 0xde, 0x79, 0xd0, 
  0x89, 0x67, 0x72, 0x4c, 0x07, 0x8d, 0x68, 0x9d, 
  0x61, 0x52, 0x1d, 0xf9, 0x2c, 0x5c, 0xba, 0x77
  };

  unsigned char calculatev_correct_output[32] = 
  {
  0x1b, 0x77, 0xb5, 0xa0, 0x44, 0x84, 0x7e, 0xb9, 
  0x23, 0xd7, 0x93, 0x18, 0xce, 0xc2, 0xc5, 0xe2, 
  0x84, 0xd5, 0x79, 0x6f, 0x65, 0x63, 0x1b, 0x60, 
  0x9b, 0xf1, 0xf8, 0xce, 0x88, 0x0b, 0x50, 0x9c,
  };

  int count;
  fe in, out;
  unsigned char bytes[32];
  fe_0(in);
  fe_0(out);
  for (count = 0; count < 32; count++) {
    bytes[count] = count;
  }
  fe_frombytes(in, bytes);
  elligator(out, in);
  fe_tobytes(bytes, out);
  TEST("Elligator vector", memcmp(bytes, elligator_correct_output, 32) == 0);

  /* Elligator(0) == 0 test */
  fe_0(in);
  elligator(out, in);
  TEST("Elligator(0) == 0", memcmp(in, out, 32) == 0);

  /* ge_montx_to_p3(0) -> order2 point test */
  fe one, negone, zero;
  fe_1(one);
  fe_0(zero);
  fe_sub(negone, zero, one);
  ge_p3 p3;
  ge_montx_to_p3(&p3, zero, 0);
  TEST("ge_montx_to_p3(0) == order 2 point", 
      fe_isequal(p3.X, zero) &&
      fe_isequal(p3.Y, negone) &&
      fe_isequal(p3.Z, one) && 
      fe_isequal(p3.T, zero));

  /* Hash to point vector test */
  unsigned char htp[32];
  
  for (count=0; count < 32; count++) {
    htp[count] = count;
  }

  hash_to_point(&p3, htp, 32);
  ge_p3_tobytes(htp, &p3);
  TEST("hash_to_point #1", memcmp(htp, hashtopoint_correct_output1, 32) == 0);

  for (count=0; count < 32; count++) {
    htp[count] = count+1;
  }

  hash_to_point(&p3, htp, 32);
  ge_p3_tobytes(htp, &p3);
  TEST("hash_to_point #2", memcmp(htp, hashtopoint_correct_output2, 32) == 0);

  /* calculate_U vector test */
  ge_p3 Bv;
  unsigned char V[32];
  unsigned char Vbuf[200];
  unsigned char a[32];
  unsigned char A[32];
  unsigned char Vmsg[3];
  Vmsg[0] = 0;
  Vmsg[1] = 1;
  Vmsg[2] = 2;
  for (count=0; count < 32; count++) {
    a[count] = 8 + count;
    A[count] = 9 + count;
  }
  sc_clamp(a);
  calculate_Bv_and_V(&Bv, V, Vbuf, a, A, Vmsg, 3);
  TEST("calculate_Bv_and_V vector", memcmp(V, calculatev_correct_output, 32) == 0);
  return 0;
}

int xeddsa_fast_test(int silent)
{
  unsigned char signature_correct[64] = {
  0x11, 0xc7, 0xf3, 0xe6, 0xc4, 0xdf, 0x9e, 0x8a, 
  0x51, 0x50, 0xe1, 0xdb, 0x3b, 0x30, 0xf9, 0x2d, 
  0xe3, 0xa3, 0xb3, 0xaa, 0x43, 0x86, 0x56, 0x54, 
  0x5f, 0xa7, 0x39, 0x0f, 0x4b, 0xcc, 0x7b, 0xb2, 
  0x6c, 0x43, 0x1d, 0x9e, 0x90, 0x64, 0x3e, 0x4f, 
  0x0e, 0xaa, 0x0e, 0x9c, 0x55, 0x77, 0x66, 0xfa, 
  0x69, 0xad, 0xa5, 0x76, 0xd6, 0x3d, 0xca, 0xf2, 
  0xac, 0x32, 0x6c, 0x11, 0xd0, 0xb9, 0x77, 0x02,
  };
  const int MSG_LEN  = 200;
  unsigned char privkey[32];
  unsigned char pubkey[32];
  unsigned char signature[64];
  unsigned char msg[MSG_LEN];
  unsigned char random[64];

  memset(privkey, 0, 32);
  memset(pubkey, 0, 32);
  memset(signature, 0, 64);
  memset(msg, 0, MSG_LEN);
  memset(random, 0, 64);

  privkey[8] = 189; /* just so there's some bits set */
  sc_clamp(privkey);
  
  /* Signature vector test */
  curve25519_keygen(pubkey, privkey);

  xed25519_sign(signature, privkey, msg, MSG_LEN, random);
  TEST("XEdDSA sign", memcmp(signature, signature_correct, 64) == 0);
  TEST("XEdDSA verify #1", xed25519_verify(signature, pubkey, msg, MSG_LEN) == 0);
  signature[0] ^= 1;
  TEST("XEdDSA verify #2", xed25519_verify(signature, pubkey, msg, MSG_LEN) != 0);
  return 0;
}

int vxeddsa_fast_test(int silent)
{
  unsigned char signature_correct[96] = {
  0x23, 0xc6, 0xe5, 0x93, 0x3f, 0xcd, 0x56, 0x47, 
  0x7a, 0x86, 0xc9, 0x9b, 0x76, 0x2c, 0xb5, 0x24, 
  0xc3, 0xd6, 0x05, 0x55, 0x38, 0x83, 0x4d, 0x4f, 
  0x8d, 0xb8, 0xf0, 0x31, 0x07, 0xec, 0xeb, 0xa0, 
  0xa0, 0x01, 0x50, 0xb8, 0x4c, 0xbb, 0x8c, 0xcd, 
  0x23, 0xdc, 0x65, 0xfd, 0x0e, 0x81, 0xb2, 0x86, 
  0x06, 0xa5, 0x6b, 0x0c, 0x4f, 0x53, 0x6d, 0xc8, 
  0x8b, 0x8d, 0xc9, 0x04, 0x6e, 0x4a, 0xeb, 0x08, 
  0xce, 0x08, 0x71, 0xfc, 0xc7, 0x00, 0x09, 0xa4, 
  0xd6, 0xc0, 0xfd, 0x2d, 0x1a, 0xe5, 0xb6, 0xc0, 
  0x7c, 0xc7, 0x22, 0x3b, 0x69, 0x59, 0xa8, 0x26, 
  0x2b, 0x57, 0x78, 0xd5, 0x46, 0x0e, 0x0f, 0x05, 
  };
  const int MSG_LEN  = 200;
  unsigned char privkey[32];
  unsigned char pubkey[32];
  unsigned char signature[96];
  unsigned char msg[MSG_LEN];
  unsigned char random[64];
  unsigned char vrf_out[32];
  unsigned char vrf_outprev[32];

  memset(privkey, 0, 32);
  memset(pubkey, 0, 32);
  memset(signature, 0, 96);
  memset(msg, 0, MSG_LEN);
  memset(random, 0, 64);

  privkey[8] = 189; /* just so there's some bits set */
  sc_clamp(privkey);

  /* Signature vector test */
  curve25519_keygen(pubkey, privkey);

  vxed25519_sign(signature, privkey, msg, MSG_LEN, random);

  TEST("VXEdDSA sign", memcmp(signature, signature_correct, 96) == 0);
  TEST("VXEdDSA verify #1", vxed25519_verify(vrf_out, signature, pubkey, msg, MSG_LEN) == 0);
  memcpy(vrf_outprev, vrf_out, 32);
  signature[0] ^= 1;
  TEST("VXEdDSA verify #2", vxed25519_verify(vrf_out, signature, pubkey, msg, MSG_LEN) != 0);

  /* Test U */
  unsigned char sigprev[96];
  memcpy(sigprev, signature, 96);
  sigprev[0] ^= 1; /* undo prev disturbance */

  random[0] ^= 1; 
  vxed25519_sign(signature, privkey, msg, MSG_LEN, random);
  TEST("VXEdDSA verify #3", vxed25519_verify(vrf_out, signature, pubkey, msg, MSG_LEN) == 0);
 
  TEST("VXEdDSA VRF value unchanged", memcmp(vrf_out, vrf_outprev, 32) == 0);
  TEST("VXEdDSA (h, s) changed", memcmp(signature+32, sigprev+32, 64) != 0);
  return 0;
}

int main(void) {
   if(0!=elligator_fast_test(0)) return 1;
   if(0!=xeddsa_fast_test(0)) return 1;
   if(0!=vxeddsa_fast_test(0)) return 1;
   return 0;
}
