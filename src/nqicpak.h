#include "nqi_common.h"
#include "nqic.h"

#ifndef _NQICPAK_H_
#define _NQICPAK_H_


typedef void* nqicpak_context;

void nqicpak_context_init(nqicpak_context *nqic, const unsigned char *src, nqic_dim *dim, unsigned int size);

bool nqicpak_encode(nqicpak_context nqic);
bool nqicpak_flush(nqicpak_context nqic);
bool nqicpak_get_output(nqicpak_context nqic, unsigned char** data, size_t* size);

#endif