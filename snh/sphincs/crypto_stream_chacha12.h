#include "chacha12/e/ref/e/ecrypt-sync.h"

#ifndef CRYPTO_STREAM_CHACHA12
#define CRYPTO_STREAM_CHACHA12

#define crypto_stream_chacha12_KEYBYTES 32
#define crypto_stream_chacha12_NONCEBYTES 8

int crypto_stream_chacha12(unsigned char *out, unsigned long long outlen, const unsigned char *n, const unsigned char *k);
int crypto_stream_chacha12_setup(ECRYPT_ctx *x, const unsigned char *n, const unsigned char *k);
int crypto_stream_chacha12_64b(ECRYPT_ctx *x, unsigned char *out);

#endif
