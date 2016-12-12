#ifndef _MD5_H
#define _MD5_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

struct MD5Context {
    uint32_t buf[4];
    uint32_t bits[2];
    unsigned char in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
    unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32_t buf[4], uint32_t const in[16]);
void MD5Hash(char *output, const char *input);
#endif /* _MD5_H */

