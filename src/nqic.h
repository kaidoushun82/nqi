#include "nqi_common.h"

#ifndef _NQIC_H_
#define _NQIC_H_

typedef void* nqic_context;

typedef struct {
    unsigned int width;
    unsigned int height;
} nqic_dim;

void nqic_init(nqic_context *nqic);
bool nqic_load_image(nqic_context nqic, const unsigned char* data, size_t size, int width, int height);
bool nqic_encode(nqic_context nqic);
bool nqic_flush(nqic_context nqic);
bool nqic_get_output(nqic_context nqic, unsigned char** data, unsigned int* size);
void nqic_free(nqic_context *nqic);

#endif