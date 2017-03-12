// run this on your host, not the pitchfork

#include <string.h>
#include <sodium.h>

void hexdump(unsigned char* prefix, unsigned char* buf, int size) {
  int i;
  printf("%20s %3d ", prefix, size);
  for(i=0;i<size;i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}

void main(void) {
  const char plaintext[]="asdf";
  int size = sizeof(plaintext) - 1;
  unsigned char padded[size+32];
  unsigned char nonce[]="\362\027\370\n\025\256{\215'm\261\332k\031\266֧\206\027\065\003\263\375\021";
  unsigned char key[]="g\207\032H\322}\366q\270\023{b\206ƴ}eǀ\327\031\354\276v\002,W#\264\323EN";
  unsigned char ciphertext[size+32];
  unsigned char plaintext2[size+32];

  memset(padded, 0, crypto_secretbox_ZEROBYTES);
  memset(padded,0,sizeof(padded));
  memset(ciphertext,0,sizeof(ciphertext));
  memset(plaintext2,0,sizeof(plaintext2));

  memcpy(padded+crypto_secretbox_ZEROBYTES, plaintext, size);

  hexdump("nonce", nonce, sizeof(nonce));
  hexdump("key", key, sizeof(key));
  hexdump("padded", padded, sizeof(padded));
  hexdump("ciphertext", ciphertext, sizeof(ciphertext));
  // encrypt
  crypto_secretbox(ciphertext, padded, size+crypto_secretbox_ZEROBYTES, nonce, key);

  size+=crypto_secretbox_ZEROBYTES -crypto_secretbox_BOXZEROBYTES ; // add nonce+mac size to total size
  hexdump("ciphertext", ciphertext, sizeof(ciphertext));
  // decrypt
  if(-1 == crypto_secretbox_open(plaintext2,  // m
                                 ciphertext, // c + preamble
                                 size+crypto_secretbox_BOXZEROBYTES,  // clen = len(plain)+2x(boxzerobytes)
                                 nonce, // n
                                 key)) {
    printf("meh\n");
    hexdump("ciphertext", ciphertext, sizeof(ciphertext));
    hexdump("plaintext2", plaintext2, sizeof(plaintext2));
    return;
  }

  printf("yippie\n");
}
