#include <string.h>
#include <stdbool.h>
#ifdef NQI_STATS
#if defined(_MSC_VER)
#include <windows.h>
#include <io.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#endif
#include "nqid.h"
#include "nqidpak.h"

typedef struct
{
    //Buffer
    unsigned char* src;
    unsigned int size;
    unsigned int curr_offs;

    //Header
    unsigned short v_maj;
    unsigned short v_min;
    bool is_animated;

    //WIM
    unsigned short frames;
    unsigned short width;
    unsigned short height;

    //Image data
    unsigned char* out_buffer;
    unsigned int out_size;

    //Decode data
    unsigned short curr_frame;
} nqid_context_internal;

#define nqid_log         //printf

#define ALIGN_TO_4(val) \
    val += ((val&3)?4-(val&3):0);

#define READ_UINT16(val, ctxt)  \
    {   \
        val = 0;    \
        val |= (ctxt->src[ctxt->curr_offs++]);    \
        val |= (ctxt->src[ctxt->curr_offs++]) << 8;    \
    }

#define READ_UINT32(val, ctxt)  \
    {   \
        val = 0;    \
        val |= (ctxt->src[ctxt->curr_offs++]);    \
        val |= (ctxt->src[ctxt->curr_offs++]) << 8;    \
        val |= (ctxt->src[ctxt->curr_offs++]) << 16;    \
        val |= (ctxt->src[ctxt->curr_offs++]) << 24;    \
    }


#define READ_VLQ16(val, ctxt, ret)     \
    {   \
        unsigned char bs = 0;   \
        unsigned char t = 0;    \
        val = 0;    \
        do  \
        {   \
            t = ctxt->src[ctxt->curr_offs]; \
            if ((val | ((t&0x7f) << bs)) > 0xffff) return (ret); \
            val |= (t&0x7f) << bs;    \
            ctxt->curr_offs++;  \
            if (t<=0x7f)   break;  \
            bs+= 7; \
        }   \
        while (true); \
    }

#define READ_VLQ32(val, ctxt, ret)     \
    {   \
        unsigned char bs = 0;   \
        unsigned char t = 0;    \
        val = 0;    \
        do  \
        {   \
            t = ctxt->src[ctxt->curr_offs]; \
            if ((val | ((t&0x7f) << bs)) > 0xffffffff) return (ret); \
            val |= (t&0x7f) << bs;    \
            ctxt->curr_offs++;  \
            if (t<=0x7f)   break;  \
            bs+= 7; \
        }   \
        while (true); \
    }

#ifdef NQI_STATS
static double now_us(void)
{
#if defined(_MSC_VER)
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart * 1e6;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1e6 + tv.tv_usec;
#endif
}
#endif

void nqid_init(nqid_context* nqid)
{
    nqid_context_internal* temp_ctxt;

    temp_ctxt = (nqid_context_internal*)malloc(sizeof(nqid_context_internal));

    if (temp_ctxt == NULL)
    {
        *nqid = NULL;
        return;
    }

    memset(temp_ctxt, 0, sizeof(nqid_context_internal));

    *nqid = (nqid_context)temp_ctxt;
}

bool nqid_load_image(nqid_context nqid, const unsigned char* data, size_t size)
{
    nqid_context_internal* temp_ctxt;
    unsigned int tsize; //no use

    if (nqid == NULL)
    {
        return false;
    }

    temp_ctxt = (nqid_context_internal*)nqid;

    if (data == NULL || size < 12) //8 for hdr + 4 for stop tag
    {
        return false;
    }

    if (!(data[0] == 'N' && data[1] == 'Q' && data[2] == 'I'))
    {
        return false;
    }

    if (data[3] == 'A')
    {
        temp_ctxt->is_animated = true;
    }
    else if (data[3] != 'M')
    {
        return false;
    }

    //version is skipped and used for encoder, although this changes in the fu...
    temp_ctxt->src = data;
    temp_ctxt->size = size;
    temp_ctxt->curr_offs = 8; //skip hdr

    //read WIM
    if (temp_ctxt->src[temp_ctxt->curr_offs++] != NQIM_META_WHOLE_IMAGE)
    {
        temp_ctxt->src = NULL;
        temp_ctxt->size = 0;
        temp_ctxt->curr_offs = 0;
        return false; //not valid
    }

    READ_VLQ32(tsize, temp_ctxt, false);
    if (tsize < 4)
    {
        temp_ctxt->src = NULL;
        temp_ctxt->size = 0;
        temp_ctxt->curr_offs = 0;
        return false; //not valid
    }

    READ_UINT16(temp_ctxt->frames, temp_ctxt);
    READ_VLQ16(temp_ctxt->width, temp_ctxt, false);
    READ_VLQ16(temp_ctxt->height, temp_ctxt, false);

    //tag is 4-byte align

    temp_ctxt->out_buffer = (unsigned char*)malloc((temp_ctxt->width * temp_ctxt->height) << 2);
    if (!temp_ctxt->out_buffer)
    {
        temp_ctxt->src = NULL;
        temp_ctxt->size = 0;
        temp_ctxt->curr_offs = 0;
        return false; //not valid
    }

    temp_ctxt->out_size = (temp_ctxt->width * temp_ctxt->height) << 2;

    ALIGN_TO_4(temp_ctxt->curr_offs);

    return true;
}

bool nqid_get_info(nqid_context nqid, unsigned short* w, unsigned short* h, unsigned short* frames)
{
    nqid_context_internal* temp_ctxt;

    if (nqid == NULL)
    {
        return false;
    }

    temp_ctxt = (nqid_context_internal*)nqid;

    if (w)
        *w = temp_ctxt->width;
    if (h)
        *h = temp_ctxt->height;
    if (frames)
        *frames = temp_ctxt->frames;

    return true;
}

bool nqid_decode(nqid_context nqid)
{
    nqid_context_internal* temp_ctxt;
    unsigned short w, h, x, y;
    unsigned int cs, ds;
    unsigned char* cp, * dp;
    unsigned int tsize; //no use

    if (nqid == NULL)
    {
        return false;
    }

    temp_ctxt = (nqid_context_internal*)nqid;

    if (temp_ctxt->curr_frame >= temp_ctxt->frames)
    {
        return false;
    }

    //read following IDM

    if (temp_ctxt->src[temp_ctxt->curr_offs++] != NQIM_META_IMAGE_DATA)
    {
        return false; //not valid
    }

    READ_VLQ32(tsize, temp_ctxt, false);
    if (tsize < 16)
    {
        return false; //not valid
    }

    temp_ctxt->curr_offs += 2; //skip idx for now...

    READ_VLQ16(w, temp_ctxt, false);
    READ_VLQ16(h, temp_ctxt, false);
    READ_VLQ16(x, temp_ctxt, false);
    READ_VLQ16(y, temp_ctxt, false);


    temp_ctxt->curr_offs += 2; //skip atts for now, none at the moment until noted...

    READ_UINT32(cs, temp_ctxt);

    cp = &temp_ctxt->src[temp_ctxt->curr_offs];

    temp_ctxt->curr_offs += cs;

    READ_UINT32(ds, temp_ctxt);

    dp = &temp_ctxt->src[temp_ctxt->curr_offs];

    temp_ctxt->curr_offs += ds * 3; //currently rgb888, changing accross updates to accomodate 565

    //tag is 4-byte align

    ALIGN_TO_4(temp_ctxt->curr_offs);

    if (x + w <= temp_ctxt->width && y + h <= temp_ctxt->height)
    {
#ifdef NQI_STATS
        double begin = now_us();
#endif
        nqidpak_decode(temp_ctxt->out_buffer, temp_ctxt->width, temp_ctxt->height, w, h, x, y, cp, dp);
#ifdef NQI_STATS
        printf("took %.0fus\n", now_us() - begin);
#endif
    }

    temp_ctxt->curr_frame++;

    return true;
}

bool nqid_get_output(nqid_context nqid, unsigned char** data, unsigned int * size)
{
    nqid_context_internal* temp_ctxt;
    if (nqid == NULL)
    {
        *data = NULL;
        *size = 0;
        return false;
    }

    temp_ctxt = (nqid_context_internal*)nqid;

    *data = temp_ctxt->out_buffer;
    *size = temp_ctxt->out_size;

    return true;
}

void nqid_free(nqid_context* nqid)
{
    nqid_context_internal* temp_ctxt;
    if (nqid == NULL)
    {
        return;
    }

    temp_ctxt = (nqid_context_internal*)nqid;

    if (temp_ctxt->out_buffer)
        free(temp_ctxt->out_buffer);

    free(temp_ctxt);
    *nqid = NULL;
}