
#ifndef ZPM_DOWNLOAD_H
#define ZPM_DOWNLOAD_H

#include <stdio.h>
#include <solv/solvable.h>

#include "zpm_repoinfo.h"

FILE * curlfopen(zpm_repoinfo *cinfo, const char *file, int uncompress, const unsigned char *chksum, Id chksumtype, int *badchecksump);

#endif
