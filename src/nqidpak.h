#include "nqi_common.h"
#include "nqic.h"

#ifndef _NQICPAK_H_
#define _NQICPAK_H_

int nqic_diff_tbl[]; //don't use extern here

void nqidpak_decode(unsigned char* out, unsigned short fw, unsigned short fh, unsigned short w, unsigned short h, unsigned short x, unsigned short y, unsigned char* c, unsigned char* d);

#endif