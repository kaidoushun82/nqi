#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
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

int nqic_diff_tbl[] = {
    0x00000000, 0x00FFFF00, 0x00000400, 0x00FFFFFF, 0x00F7FF00, 0x00FF0000, 0x00FFFBFF, 0x00000008,
    0x00080000, 0x00000408, 0x00080400, 0x00080808, 0x00F7F7FF, 0x00F7FBFF, 0x00080408, 0x00F7FB00,
    0x00FFF7FF, 0x00000808, 0x00FFFB00, 0x00080C08, 0x00080800, 0x000000FF, 0x00080C10, 0x00FFFF08,
    0x00F7F3FF, 0x00000800, 0x00F7FFFF, 0x00FF0400, 0x00080810, 0x00101010, 0x0000FF00, 0x00F7F3F7,
    0x00080008, 0x00100C08, 0x00100C10, 0x0000FFFF, 0x00EFEFF7, 0x00EFF3FF, 0x00FF0008, 0x00F7F7F7,
    0x00EFF7FF, 0x00000410, 0x00081010, 0x00EFFF00, 0x00EFFB00, 0x0000FBFF, 0x00100808, 0x000800FF,
    0x00101418, 0x00000810, 0x00101008, 0x00EFF3F7, 0x00F7FF08, 0x00FF0408, 0x00F7EFF7, 0x00FFF3FF,
    0x00101410, 0x00100800, 0x00FFF7F7, 0x00101018, 0x00100400, 0x00EFEBEF, 0x00EFEFFF, 0x00FFFBF7,
    0x00EFEBF7, 0x00000010, 0x00000C08, 0x00F7F700, 0x00181818, 0x00EFEFEF, 0x00080410, 0x00FFF3F7,
    0x00F70000, 0x00080C00, 0x00E7E7EF, 0x00EFF700, 0x00081008, 0x00EFFBFF, 0x0000F7FF, 0x00F7EFFF,
    0x00000C10, 0x00FFFFF7, 0x00181418, 0x00101818, 0x00081018, 0x00181C21, 0x00100C00, 0x00080C18,
    0x00100000, 0x00181410, 0x00E7E3E7, 0x00F7FBF7, 0x00100408, 0x00100810, 0x00181821, 0x00EFE7EF,
    0x00181C18, 0x00FF0808, 0x00E7EBEF, 0x00181008, 0x00E7E7E7, 0x00E7FB00, 0x00181810, 0x00E7EBF7,
    0x00F7EFEF, 0x00101421, 0x0000FB00, 0x00FF00FF, 0x00101821, 0x00FFEFF7, 0x000004FF, 0x00FF0800,
    0x00E7F3FF, 0x00212021, 0x00FFF700, 0x00FFFB08, 0x00181408, 0x00E7EFFF, 0x00EFF7F7, 0x00F7F3EF,
    0x00181010, 0x00101408, 0x00E7E3EF, 0x00100C18, 0x00E7F700, 0x00E7FF00, 0x00F7FB08, 0x00180C08,
    0x00081418, 0x00180800, 0x00E7E7F7, 0x000804FF, 0x00F70008, 0x00DEDFE7, 0x00F7EBF7, 0x00000C00,
    0x00212429, 0x0000FF08, 0x0008FFFF, 0x00EFEBFF, 0x00E7EFF7, 0x00EFFF08, 0x00E7DFE7, 0x00101810,
    0x00081410, 0x00000418, 0x00180C00, 0x00EFE7E7, 0x00EFE7F7, 0x00000818, 0x00F70400, 0x00F7EBEF,
    0x0000F3FF, 0x001000FF, 0x00DEDBDE, 0x00EFEBE7, 0x00FFFF10, 0x00FF0410, 0x00211C21, 0x000000F7,
    0x00080818, 0x00E7F7FF, 0x00211C18, 0x00001010, 0x00DEDFDE, 0x00EFF3EF, 0x00182021, 0x00FFEFFF,
    0x00180400, 0x00E7EBFF, 0x00000C18, 0x00EFF300, 0x00212029, 0x00182029, 0x00DEDFEF, 0x00212421,
    0x00DEE3E7, 0x00212018, 0x00101000, 0x00DEE3EF, 0x00E7DFDE, 0x0000F7F7, 0x00292829, 0x00DEDBE7,
    0x00F70408, 0x0008FF00, 0x00E7E3DE, 0x00EFFFFF, 0x00FF0810, 0x00211810, 0x0000F3F7, 0x00FF0010,
    0x0008FBFF, 0x00211C10, 0x00181C10, 0x00E7F300, 0x0000FBF7, 0x00181000, 0x00DEF700, 0x00211808,
    0x00FFF7EF, 0x00081421, 0x00101C21, 0x00181808, 0x00DEFB00, 0x001004FF, 0x00000018, 0x00DED7D6,
    0x00FFEFEF, 0x00180808, 0x00EF0000, 0x00211408, 0x00EFFB08, 0x00FFF3EF, 0x00F7F300, 0x00181C29,
    0x00101C18, 0x00101021, 0x00001008, 0x00292C31, 0x00FFFBEF, 0x00081818, 0x00181421, 0x00DEDBD6,
    0x00FFEBF7, 0x00DEEFFF, 0x00181018, 0x00DED7DE, 0x00212829, 0x00D6D7DE, 0x00F7F7EF, 0x00211818,
    0x00182018, 0x0000EFF7, 0x00081021, 0x00FF0C08, 0x00E7E3F7, 0x00100008, 0x00E7EBE7, 0x00001018,
    0x00D6D7D6, 0x0000FFF7, 0x00212010, 0x00DEF300, 0x00211008, 0x00081000, 0x00DEE7F7, 0x00292831,
    0x00E7DFEF, 0x00212831, 0x00EFE3E7, 0x00DEF3FF, 0x00DEE3F7, 0x00E7E7FF, 0x00E7F3F7, 0x00211410,
    0x00D6D3D6, 0x00E7EFEF, 0x00212431, 0x00DEEBFF, 0x00291C08, 0x00D6DBDE, 0x00FFFFEF, 0x00DEE7EF,
    0x00DEDFF7, 0x00081821, 0x00180000, 0x00F7EBFF, 0x00080010, 0x00FFEBEF, 0x00000421, 0x00180C10,
    0x00F7F708, 0x00292C29, 0x00D6D3DE, 0x00EFE3EF, 0x00210C00, 0x00292821, 0x00CECFD6, 0x00292421,
    0x00E7DBDE, 0x00E7FF08, 0x00F7FF10, 0x00DEDBEF, 0x00E7FBFF, 0x00081408, 0x000800F7, 0x00292418,
    0x00FF0C10, 0x00181400, 0x00D6D7E7, 0x00212418, 0x00DEFF00, 0x00F7E7EF, 0x00210800, 0x00DEEBF7,
    0x00211000, 0x00D6DBE7, 0x00EFEFE7, 0x00D6D3CE, 0x00F7FFF7, 0x00D6DBEF, 0x00292429, 0x00211C08,
    0x00F7EFE7, 0x00FF0818, 0x00313031, 0x00EF0008, 0x00D6F700, 0x00D6DFEF, 0x00182429, 0x00F70808,
    0x00F7EBE7, 0x00000821, 0x00EFF708, 0x0008F7FF, 0x001800FF, 0x00212821, 0x00FFFBE7, 0x0000EFFF,
    0x00000021, 0x00101C29, 0x00291C10, 0x00101808, 0x00211400, 0x00291808, 0x00E7E7DE, 0x001804FF,
    0x00292018, 0x00DEE3DE, 0x00212C31, 0x00E7FB08, 0x00293039, 0x00FF0418, 0x00FFF7E7, 0x00312008,
    0x00EFEF00, 0x00E7DBE7, 0x00E7EF00, 0x00210C08, 0x00001410, 0x00211C29, 0x00D6F300, 0x0010FFFF,
    0x00D6CFD6, 0x00D6CFCE, 0x00EFE7FF, 0x00180408, 0x00101829, 0x00292010, 0x00DEE7FF, 0x00CECBCE,
    0x00292C21, 0x00CECFDE, 0x00100410, 0x0000F700, 0x00CED3DE, 0x00F7E7F7, 0x00313439, 0x00DEEF00,
    0x00293031, 0x00292008, 0x001008FF, 0x00181829, 0x00101400, 0x00EFFBF7, 0x00CECBD6, 0x00DEF7FF,
    0x00CECFCE, 0x00001418, 0x00292C39, 0x00211800, 0x00000C21, 0x00EFE7DE, 0x00C6C7CE, 0x00081810,
    0x00313431, 0x0000EBF7, 0x00312C29, 0x00210400, 0x00F70800, 0x00080C21, 0x00CEC7C6, 0x00DED7E7,
    0x00D6DFE7, 0x00EF0400, 0x00100818, 0x00F7E7E7, 0x00182421, 0x00FF0C00, 0x00313029, 0x00DED3D6,
    0x000808FF, 0x00101C10, 0x00292021, 0x00CED3D6, 0x00D6EFFF, 0x00312C21, 0x00D6FB00, 0x00FFFF18,
    0x00FFE7EF, 0x00E7DBD6, 0x00EF0408, 0x00FF0C18, 0x00EFE3DE, 0x00182431, 0x00D6D7EF, 0x00D6EF00,
    0x00D6EBFF, 0x00DED3DE, 0x00313039, 0x00313442, 0x00E7DFF7, 0x00F70010, 0x00181C08, 0x00292410,
    0x00001021, 0x00D6CBCE, 0x00F70410, 0x00102021, 0x00292818, 0x00DEDFD6, 0x00C6C3CE, 0x00CECBC6,
    0x00EFE3F7, 0x00211010, 0x00211821, 0x00393839, 0x00EFDFDE, 0x00D6D3E7, 0x00FFF7DE, 0x00080418,
    0x00312408, 0x00D6DFF7, 0x00D6CFDE, 0x00291800, 0x00313842, 0x000000EF, 0x00291408, 0x00CED3E7,
    0x0010087B, 0x00313018, 0x00F7F3E7, 0x00FFF3E7, 0x001808FF, 0x00101429, 0x00291810, 0x00291000,
    0x00FF0018, 0x00DEE7E7, 0x00DEEFF7, 0x00312C31, 0x0008FB00, 0x00293029, 0x00C6C3C6, 0x00FFF708,
    0x00DEFF08, 0x00E7DFD6, 0x00C6CBD6, 0x000004F7, 0x00290C00, 0x00182010, 0x00FFEBFF, 0x00001421,
    0x00311C08, 0x00393831, 0x0010FBFF, 0x00CEF300, 0x00FFF300, 0x00FFFBDE, 0x00D6E3F7, 0x00313021,
    0x00291C18, 0x00001000, 0x0000EBEF, 0x00102029, 0x00CEC7CE, 0x00000429, 0x00291400, 0x00BDBEC6,
    0x00CECBDE, 0x00E7F708, 0x00312410, 0x00D6E3EF, 0x00FFFFE7, 0x00FFFB10, 0x00212410, 0x00EFF308,
    0x00182418, 0x00312C18, 0x00313429, 0x00524DA5, 0x00312818, 0x0000F3EF, 0x00F7FBEF, 0x00D6E7FF,
    0x00F70810, 0x00393C39, 0x00212031, 0x00DED7CE, 0x00EFDFE7, 0x00FFE7F7, 0x00000829, 0x00181800,
    0x00FFEFE7, 0x00CECFE7, 0x00CED7E7, 0x00F7F308, 0x00BDBEE7, 0x00C6C7D6, 0x00E70000, 0x00EFFF10
};

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
    unsigned char* out;
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

typedef struct
{
    const uint8_t* lit;
    const uint8_t* cur;
    int offs;
} nqi_pels;

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

#ifdef _MSC_VER
#define NQI_INLINE static __forceinline
#else
#define NQI_INLINE static inline __attribute__((always_inline))
#endif

#define      BSWAP1(tmp) \
{\
  register uint32_t  t; \
  t = tmp ^ ((tmp << 16) | (tmp >> 16)); \
  t &= ~0xff0000; \
  tmp = (tmp << 24) | (tmp >> 8); \
  tmp = tmp ^ (t >> 8); \
}

typedef struct {
    const uint8_t* p;
    const uint8_t* end;
    uint32_t acc;
    int n;
} nqi_br;

NQI_INLINE uint32_t br_get(nqi_br* b, uint32_t k)
{
    uint32_t v;
    if (b->n == 0)
    {
        memcpy(&b->acc, b->p, 4);
        b->p += 4;
        BSWAP1(b->acc);
        b->n = 32-k;
        v = (b->acc >> b->n);
        b->acc <<= k;
    }
    else if (b->n < k)
    {
        char f = (k - b->n);
        v = (b->acc >> (32 - b->n)) << f;
        memcpy(&b->acc, b->p, 4);
        b->p+=4;
        BSWAP1(b->acc);
        b->n = 32 - f;
        v |= (b->acc >> b->n);
        b->acc <<= f;
    }
    else
    {
        v = (b->acc >> (32 - k));
        b->acc <<= k;
        b->n -= k;
    }
    return v;
}

NQI_INLINE unsigned br_eg(nqi_br* b)
{
    unsigned v = 1, stop;
    bool to_fill = br_get(b, 1);
    if (!to_fill)
        return v;
    do
    {
        v = (v << 1) | br_get(b, 1);
        stop = br_get(b, 1);
    } while (!stop);
    return v;
}

NQI_INLINE int br_int4(nqi_br* b)
{
    int v;
    if (br_get(b, 1) == 0)
        v = (int)br_get(b, 9);
    else
    {
        unsigned s = br_get(b, 3) | 8;
        if (s == 15)
        {
            s += br_get(b, 1);
            if (s == 16) s += br_get(b, 1);
        }
        v = (int)(((2u << s) - 2) + br_get(b, s + 1));
    }
    return br_get(b, 1) ? -v : v;
}

NQI_INLINE uint32_t px_add(uint32_t a, uint32_t b)
{
    return ((a & 0x7f7f7f7fu) + (b & 0x7f7f7f7fu)) ^ ((a ^ b) & 0x80808080u);
}

NQI_INLINE uint32_t px_sub(uint32_t a, uint32_t b)
{
    return ((a | 0x80808080u) - (b & 0x7f7f7f7fu)) ^ ((a ^ ~b) & 0x80808080u);
}

NQI_INLINE uint32_t px_load(const uint8_t* p)
{
    return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | 0xff000000u;
}

NQI_INLINE uint32_t px_stream(nqi_br* b, nqi_pels* s)
{
    if (br_get(b, 1))
    {
        uint32_t v = px_load(s->cur);
        s->cur += 3;
        return v;
    }
    s->offs += br_int4(b);
    return px_load(s->cur - (ptrdiff_t)s->offs * 3);
}

#define POS(i) (((i) >> 2) * stride + ((i) & 3))

NQI_INLINE void block_rle(nqi_br* b, nqi_pels* s, uint32_t* dst, ptrdiff_t stride)
{
    unsigned i = 0;
    do
    {
        unsigned end;
        uint32_t v;
        end = i + br_eg(b);
        if (end > 16) end = 16;
        v = px_stream(b, s);
        do { dst[POS(i)] = v; } while (++i < end);
    } while (i < 16);
}

NQI_INLINE void block_delta(nqi_br* b, nqi_pels* s, uint32_t* dst, ptrdiff_t stride, ptrdiff_t ro)
{
    unsigned i = 0;
    do
    {
        unsigned end;
        uint32_t v, delta;
        end = i + br_eg(b);
        if (end > 16) end = 16;

        if (br_get(b, 1))
        {
            do { dst[POS(i)] = dst[POS(i) + ro]; } while (++i < end);
            continue;
        }

        if (br_get(b, 1))
            v = px_add(dst[POS(i) + ro], (uint32_t)nqic_diff_tbl[br_eg(b)]);
        else
            v = px_stream(b, s);

        delta = px_sub(v, dst[POS(i) + ro]);
        dst[POS(i)] = v;
        while (++i < end)
            dst[POS(i)] = px_add(dst[POS(i) + ro], delta);
    } while (i < 16);
}

NQI_INLINE void block_decode(nqi_br* b, nqi_pels* s, uint32_t* dst, ptrdiff_t stride, unsigned t)
{
    if (t == 0)
        block_rle(b, s, dst, stride);
    else if (t == 1)
        block_delta(b, s, dst, stride, -4);
    else if (t == 2)
    {
        unsigned typ = br_get(b, 2);
        ptrdiff_t ro = (typ == 0) ? -1 : (typ == 1) ? -stride : -stride - 1;
        block_delta(b, s, dst, stride, ro);
    }
    else
    {
        int r;
        for (r = 0; r < 4; r++, dst += stride)
            dst[0] = dst[1] = dst[2] = dst[3] = dst[-1];
    }
}

static void block_edge(nqi_br* b, nqi_pels* s, uint32_t* org, ptrdiff_t stride,
    int w, int h, int x0, int y0, unsigned t)
{
    uint32_t win[5 * 8] = { 0 };
    uint32_t* dst = win + 8 + 4;
    int r, c;

    for (r = -1; r < 4; r++)
        for (c = -4; c < 4; c++)
        {
            int px = x0 + c, py = y0 + r;
            if ((r >= 0 && c >= 0) || px < 0 || py < 0)
                continue;
            if (px >= w) px = w - 1;
            if (py >= h) py = h - 1;
            dst[r * 8 + c] = org[py * stride + px];
        }

    block_decode(b, s, dst, 8, t);

    for (r = 0; r < 4 && y0 + r < h; r++)
        for (c = 0; c < 4 && x0 + c < w; c++)
            org[(y0 + r) * stride + x0 + c] = dst[r * 8 + c];
}

void _nqid_decode_frame(unsigned char* out, unsigned short fw, unsigned short fh, unsigned short w, unsigned short h, unsigned short x, unsigned short y, unsigned char* c, unsigned char* d)
{
    const ptrdiff_t stride = fw;
    uint32_t* org = (uint32_t*)out + (size_t)y * fw + x;
    nqi_br b = { c, d, 0, 0 };
    nqi_pels s = { d, d, 0 };
    int x0, y0, r, k;

    (void)fh;

    for (y0 = 0; y0 < h; y0 += 4)
    {
        const int rows = (h - y0 < 4) ? h - y0 : 4;
        uint32_t* line = org + y0 * stride;

        if (br_get(&b, 1))
        {
            for (r = 0; r < rows; r++)
                memcpy(line + r * stride, line + (r - 4) * stride, (size_t)w * 4);
            continue;
        }

        for (x0 = 0; x0 < w; )
        {
            unsigned t;
            if (br_get(&b, 1))
            {
                int end = x0 + 4 * (int)br_eg(&b);
                int stop = (end < w) ? end : w;
                for (r = 0; r < rows; r++)
                {
                    uint32_t* p = line + r * stride;
                    for (k = x0; k + 4 <= stop; k += 4)
                        memcpy(p + k, p + k - 4, 16);
                    for (; k < stop; k++)
                        p[k] = p[k - 4];
                }
                x0 = end;
                continue;
            }

            t = br_get(&b, 2);
            if (x0 + 4 <= w && rows == 4)
                block_decode(&b, &s, line + x0, stride, t);
            else
                block_edge(&b, &s, org, stride, w, h, x0, y0, t);
            x0 += 4;
        }
    }
}

#undef POS

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

    temp_ctxt->out = (unsigned char*)malloc((temp_ctxt->width * temp_ctxt->height) << 2);
    if (!temp_ctxt->out)
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
        _nqid_decode_frame(temp_ctxt->out, temp_ctxt->width, temp_ctxt->height, w, h, x, y, cp, dp);
#ifdef NQI_STATS
        printf("took %.0fus\n", now_us() - begin);
#endif
    }

    temp_ctxt->curr_frame++;

    return true;
}

bool nqid_get_output(nqid_context nqid, unsigned char** data)
{

}

void nqid_free(nqid_context* nqid)
{

}