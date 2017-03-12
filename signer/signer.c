#include <stdint.h>
#include "blake512.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypto_sign.h>
#include <string.h>

const unsigned char sk[64];

int main(int argc, char *argv[]) {
  int i;
  if(argc<2) {
    fprintf(stderr, "%s image.bin\n",argv[0]);
    exit(1);
  }
  // todo check argv for malicious paths
  FILE *f = fopen(argv[1], "r");
  uint8_t buf[128*1024];
  memset(buf,0xff,sizeof(buf));
  size_t size;
  if((size = fread(buf,1,sizeof(buf)-crypto_sign_BYTES,f)) <= 0) {
    fprintf(stderr, "error reading file: %s\n", argv[1]);
    exit(1);
  };
  fprintf(stderr, "[i] read %d bytes\n", size);

  fprintf(stderr, "[i] patching bytes at section boundary\n", size);
  for(i=0;i<4;i++) {
    buf[0x184+i]=0xff;
  }
  FILE *o = fopen("xxx", "w");
  fwrite(buf,1,sizeof(buf)-crypto_sign_BYTES,o);
  fclose(o);

  fprintf(stderr, "[+] hashing %d bytes... ", sizeof(buf)-crypto_sign_BYTES);
  uint8_t digest[BLAKE512_BYTES];
  crypto_hash_blake512(digest, buf, sizeof(buf)-crypto_sign_BYTES);

  fprintf(stderr, "done\n");
  fprintf(stderr, "const unsigned char digest[]={0x%02x", digest[0]);
  for(i=1;i<sizeof(digest);i++) {
    fprintf(stderr, ", 0x%02x", digest[i]);
  }
  fprintf(stderr, "};\n");

  fprintf(stderr, "[+] signing... ");
  uint8_t sig[crypto_sign_BYTES];
  crypto_sign_detached(sig, NULL, digest, sizeof(digest), sk);
  fprintf(stderr, "done\n");

  printf("const unsigned char __attribute__((section (\".sigSection\"))) firmware_sig[]={0x%02x", sig[0]);

  for(i=1;i<sizeof(sig);i++) {
    printf(", 0x%02x", sig[i]);
  }
  printf("};\n");
  fclose(f);

  return 0;
}
