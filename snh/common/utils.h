#ifndef UTILS_H
#define UTILS_H

typedef unsigned char u8;

void * zerobytes(u8 *v,size_t n);
int cmp(const void * a, const void *b, const size_t size);

#endif // UTILS_H
