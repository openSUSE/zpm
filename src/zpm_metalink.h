
#ifndef ZPM_METALINK_H
#define ZPM_METALINK_H

#include <stdio.h>
#include <solv/solvable.h>

void findfastest(char **urls, int nurls);
char * findmetalinkurl(FILE *fp, unsigned char *chksump, Id *chksumtypep);
char * findmirrorlisturl(FILE *fp);

#endif
