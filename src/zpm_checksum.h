
#ifndef ZPM_CHECKSUM_H
#define ZPM_CHECKSUM_H

#include <stdio.h>

extern "C" {
#include <solv/solvable.h>
}

int verify_checksum(int fd, const char *file, const unsigned char *chksum, Id chksumtype);
int checksig(Pool *sigpool, FILE *fp, FILE *sigfp);
int checksig(Pool *sigpool, FILE *fp, FILE *sigfp);
void calc_checksum_fp(FILE *fp, Id chktype, unsigned char *out);
void calc_checksum_stat(struct stat *stb, Id chktype, unsigned char *out);

#endif
