#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "nqic.h"
#include "nqicpak.h"

typedef struct {
    //Source
    unsigned char** src_buffers;
    unsigned int* src_sizes;
    nqic_dim** src_dims;
    unsigned short cnt;

    //Encoder
    unsigned char* out_buffer;
    unsigned int out_size;
    unsigned int init_width;
    unsigned int init_height;
    nqim_metadata** metas;
    unsigned int metas_count;
    bool flushed;

} nqic_context_internal;

void nqic_init(nqic_context *nqic)
{
    nqic_context_internal *temp_ctxt;

    temp_ctxt = (nqic_context_internal*)malloc(sizeof(nqic_context_internal));

    if (temp_ctxt == NULL)
    {
        *nqic = NULL;
        return;
    }

    memset(temp_ctxt, 0, sizeof(nqic_context_internal));

    *nqic = (nqic_context)temp_ctxt;
}

bool nqic_load_image(nqic_context nqic, const unsigned char* data, size_t size, int width, int height)
{
    nqic_context_internal *temp_ctxt;
    int new_cnt;
    nqic_dim** new_dims;
    unsigned char** new_src_buffers;
    unsigned int* new_src_sizes;   
    nqic_dim* new_dim;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqic_context_internal*)nqic;

    //count is MAXIMUM uint16, so check if the limit is over that
    new_cnt = temp_ctxt->cnt + 1;
    if (new_cnt > USHRT_MAX)
        goto clean_me;

    //add to buffers array
    {    
        new_src_buffers = (unsigned char**)malloc(sizeof(unsigned char*)*new_cnt);
        if (new_src_buffers == NULL)
            goto clean_me;
        memcpy(new_src_buffers,temp_ctxt->src_buffers,sizeof(unsigned char*)*temp_ctxt->cnt);
        if (data == NULL)
            goto clean_me;
        new_src_buffers[temp_ctxt->cnt] = data;
    }

    //add to dims array
    {    
        new_dims = (nqic_dim**)realloc(temp_ctxt->src_dims, sizeof(nqic_dim*)*new_cnt);
        if (new_dims == NULL)
            goto clean_me;
        memcpy(new_dims,temp_ctxt->src_dims,sizeof(nqic_dim*)*temp_ctxt->cnt);
        new_dim = (nqic_dim*)malloc(sizeof(nqic_dim));
        if (new_dim == NULL)
            goto clean_me;
        if (width <= 0 || height <= 0)
            goto clean_me;
        new_dim->width = width;
        new_dim->height = height;
        new_dims[temp_ctxt->cnt] = new_dim;
    }

    //add to sizes array
    {    
        new_src_sizes = (unsigned int*)malloc(sizeof(unsigned int)*new_cnt);
        if (new_src_sizes == NULL)
            goto clean_me;
        memcpy(new_src_sizes,temp_ctxt->src_sizes,sizeof(unsigned int)*temp_ctxt->cnt);
        if (size == 0)
            goto clean_me;
        new_src_sizes[temp_ctxt->cnt] = size;
    }

    //change our pointers to the new ones, free old ones before setting them
    if (temp_ctxt->src_buffers != NULL)
        free(temp_ctxt->src_buffers);
    temp_ctxt->src_buffers = new_src_buffers;
    if (temp_ctxt->src_dims != NULL)
        free(temp_ctxt->src_dims);
    temp_ctxt->src_dims = new_dims;
    if (temp_ctxt->src_sizes != NULL)
        free(temp_ctxt->src_sizes);
    temp_ctxt->src_sizes = new_src_sizes;
    temp_ctxt->cnt = new_cnt;
    ret = true;

    //cleanup is useful only if a condition wasn't respected (return remains false from lack of memory or invalid parameter)
clean_me:
    if (ret == false)
    {
        if (new_src_buffers != NULL)
            free(new_src_buffers);
        if (new_dims != NULL)
            free(new_dims);  
        if (new_src_sizes != NULL)
            free(new_src_sizes);   
        if (new_dim != NULL)
            free(new_dim);
    }

    return ret;
}

bool nqic_encode(nqic_context nqic)
{
    nqic_context_internal *temp_ctxt;
    unsigned int current_image = 0;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqic_context_internal*)nqic;

    if (temp_ctxt->cnt == 0)
    {
        goto clean_me;
    }

    while (current_image < temp_ctxt->cnt)
    {
        nqicpak_context nqicpak;

        nqicpak_context_init(&nqicpak, temp_ctxt->src_buffers[current_image], temp_ctxt->src_dims[current_image], temp_ctxt->src_sizes[current_image]);
        nqicpak_encode(nqicpak);
        nqicpak_flush(nqicpak);

        current_image++;
    }

clean_me:
    return ret;
}

bool nqic_flush(nqic_context nqic)
{
    nqic_context_internal *temp_ctxt;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqic_context_internal*)nqic;

    //do flushing a bit later :)
clean_me:
    return ret;

}

bool nqic_get_output(nqic_context nqic, unsigned char** data, size_t* size)
{
    nqic_context_internal *temp_ctxt;
    if (nqic == NULL)
    {
        *data = NULL;
        *size = 0;
        return false;
    }

    temp_ctxt = (nqic_context_internal*)nqic;

    if (temp_ctxt->flushed == false)
    {
        *data = NULL;
        *size = 0;
        return false;
    }

    *data = temp_ctxt->out_buffer;
    *size = temp_ctxt->out_size;

    return true;
}

void nqic_free(nqic_context *nqic)
{ }