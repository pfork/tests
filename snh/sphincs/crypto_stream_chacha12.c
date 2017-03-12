#include "crypto_stream_chacha12.h"
#include <stddef.h>

#include "chacha12/e/ref/e/chacha.c"

int crypto_stream_chacha12(unsigned char *out, unsigned long long outlen, const unsigned char *n, const unsigned char *k) 
{
    ECRYPT_ctx x;
    ECRYPT_keysetup(&x, k, crypto_stream_chacha12_KEYBYTES*8, crypto_stream_chacha12_NONCEBYTES*8);
    ECRYPT_ivsetup(&x, n);
    ECRYPT_keystream_bytes(&x, out, outlen);
    return 0;
}

int crypto_stream_chacha12_setup(ECRYPT_ctx *x, const unsigned char *n, const unsigned char *k) {
    ECRYPT_keysetup(x, k, 256, 64);
    ECRYPT_ivsetup(x, n);
    return 0;
}

int crypto_stream_chacha12_64b(ECRYPT_ctx *x, unsigned char *out)
{
    // this cannot be generalised immediately for non-divisors of outlen
    ECRYPT_keystream_bytes(x, out, 64);
    return 0;
}
