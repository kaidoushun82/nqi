#include "nqi_common.h"

#ifndef _NQID_H_
#define _NQID_H_

typedef void* nqid_context;

void nqid_init(nqid_context *nqid);
bool nqid_load_image(nqid_context nqid, const unsigned char* data, size_t size);
bool nqid_get_info(nqid_context nqid, unsigned short* w, unsigned short* h, unsigned short* frames);
bool nqid_decode(nqid_context nqid); //NOTE: decode is only forwards, backwards makes decode time SLOW 
bool nqid_get_output(nqid_context nqid, unsigned char** data); //taking in account we know W and H
void nqid_free(nqid_context *nqid);

#endif