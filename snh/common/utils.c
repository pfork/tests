#include <sys/types.h>
#include "utils.h"

void* zerobytes(u8 *v,size_t n) {
  volatile u8 *p=v; while (n--) *p++=0; return v;
}

int cmp(const void * a, const void *b, const size_t size) {
  const unsigned char *_a = (const unsigned char *) a;
  const unsigned char *_b = (const unsigned char *) b;
  unsigned char result = 0;
  size_t i;

  for (i = 0; i < size; i++) {
    result |= _a[i] ^ _b[i];
  }

  return result; /* returns 0 if equal, nonzero otherwise */
}
