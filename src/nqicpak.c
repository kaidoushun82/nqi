#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "nqicpak.h"
#include "nqichash.h"

/* src: https://stackoverflow.com/a/20468180 */

#ifdef _MSC_VER
#include <intrin.h>

uint32_t __inline __builtin_clz(uint32_t value)
{
    unsigned int leading_zero = 0;

    if (_BitScanReverse(&leading_zero, value))
    {
        return 31 - leading_zero;
    }
    else
    {
        return 32;
    }
}
#endif

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

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} nqicpak_rgba;

typedef struct {
    nqicpak_rgba pels[256];
    unsigned int w;
    unsigned int h;
    unsigned char cmd[4096];
    unsigned int cmd_size;
    unsigned char pelsw[768];
    unsigned int pelsw_size;
} nqicpak_block;

typedef struct {
    //Source image
    unsigned char* src;
    unsigned int src_size;
    nqic_dim* src_dim;

    //Blocks
    nqicpak_block** blocks;
    unsigned int blocks_cnt;

    //Cmd
    unsigned char* cmd;
    unsigned int cmd_size;

    //Pixels
    unsigned char* pels;
    unsigned int pels_size;

    //Hashing
    nqi_pelhash ph;
    nqi_dtmap dt;

    //diff
    int curr_diff;
} nqicpak_context_internal;

void _nqicpak_write_bits(unsigned char* encoded_bytes, unsigned int* encoded_bytes_len, unsigned int bits, int num_bits)
{
    int byte_idx = *encoded_bytes_len >> 3;
    int bit_idx = *encoded_bytes_len & 7;
    int i;
    for (i = 0; i < num_bits; i++)
    {
        if (bit_idx == 0)
            encoded_bytes[byte_idx] = 0;
        if (bits & (1 << (num_bits - i - 1)))
            encoded_bytes[byte_idx] |= (1 << (7 - bit_idx));
        bit_idx++;
        if (bit_idx == 8)
        {
            bit_idx = 0;
            byte_idx++;
        }
    }
    *encoded_bytes_len += num_bits;
}

void _nqicpak_write_bit(unsigned char* encoded_bytes, unsigned int* encoded_bytes_len, unsigned char bit)
{
    _nqicpak_write_bits(encoded_bytes, encoded_bytes_len, bit, 1);
}

void _nqicpak_read_block_4x4(nqicpak_block* block, const unsigned char* src, nqic_dim* dim, int x, int y)
{
    unsigned char* block_start = (unsigned char*)src + ((y * dim->width + x) * 4);
    unsigned int c_x = 0, c_y = 0;
    //we assume that block is allocated and that src is valid

    block->w = block->h = 4;

    while (c_y < 4)
    {
        if (c_y + y >= dim->height)
        {
            //fill with 0s
            while (c_x < 4)
            {
                nqicpak_rgba* pel = &block->pels[c_y * 4 + c_x];
                pel->r = 0;
                pel->g = 0;
                pel->b = 0;
                pel->a = 0;
                c_x++;
            }
            c_x = 0;
            c_y++;
            block->h--;
            continue;
        }
        while (c_x < 4)
        {
            if (c_x + x >= dim->width)
            {
                //fill with 0s
                nqicpak_rgba* pel = &block->pels[c_y * 4 + c_x];
                pel->r = 0;
                pel->g = 0;
                pel->b = 0;
                pel->a = 0;
                c_x++;
                if (c_y == 0) block->w--;
                continue;
            }
            unsigned char* pixel = block_start + ((c_y * dim->width + c_x) * 4);
            nqicpak_rgba* pel = &block->pels[c_y * 4 + c_x];
            memcpy(pel, pixel, sizeof(nqicpak_rgba));
            c_x++;
        }
        c_x = 0;
        c_y++;
    }
}

bool _nqicpak_equal_pixel_4x4(nqicpak_rgba p1, nqicpak_rgba p2)
{
    return ((p2.r == p1.r) && (p2.g == p1.g) && (p2.b == p1.b)); //returns 0 if candidate success
}


int _nqicpak_find_pixel_4x4(nqicpak_block* block, nqicpak_rgba pixel, unsigned int curr_pel)
{
    int p = 0;
    while (p < 4 * 4 && p < curr_pel)
    {
        if (_nqicpak_equal_pixel_4x4(block->pels[p], pixel) == true)
            return p;
        p++;
    }
    return -1;
}

int _nqicpak_find_pixel_4x4_rgba(nqicpak_rgba* block, nqicpak_rgba pixel, unsigned int curr_pel)
{
    int p = 0;
    while (p < 4 * 4 && p < curr_pel)
    {
        if (_nqicpak_equal_pixel_4x4(block[p], pixel) == true)
            return p;
        p++;
    }
    return -1;
}

int _nqicpak_find_rle_4x4(nqicpak_block* block, unsigned int curr_pel)
{
    int p = curr_pel + 1;
    while (p < 4 * 4)
    {
        if (_nqicpak_equal_pixel_4x4(block->pels[curr_pel], block->pels[p]) == false)
            break;
        p++;
    }
    return p - curr_pel;
}

int _nqicpak_find_rle_4x4_rgba(nqicpak_rgba* block, unsigned int curr_pel)
{
    int p = curr_pel + 1;
    while (p < 4 * 4)
    {
        if (_nqicpak_equal_pixel_4x4(block[curr_pel], block[p]) == false)
            break;
        p++;
    }
    return p - curr_pel;
}

static inline uint32_t nqi_idx_base(unsigned s) { return (2u << s) - 2u; }

#define NQI_IDX_MAX 508

/*
Offset idx uses 4bit+rem integer encoding 
For 15+ bits, we add some more bits to save costs a bit... If < 15 bits, then no need
*/

static int nqi_idx_split4(uint32_t index, unsigned* shift, uint32_t* extra)
{
    if (index > 0x5FFFF)
        return -1;

    unsigned s = (31u - (unsigned)__builtin_clz(index + 2u)) - 1u;

    *shift = s;
    *extra = index - nqi_idx_base(s);
    return 0;
}

static int _nqicpak_pack_int4(unsigned char* cmd, unsigned int* cmd_size, int n)
{
    unsigned s; uint32_t e;
    unsigned int index_unsigned = ((n < 0) ? (n * (-1)) : n);
    if (nqi_idx_split4(index_unsigned, &s, &e)) return -1;
    if (s < 15)
        _nqicpak_write_bits(cmd, cmd_size, s, 4);
    else
    {
        _nqicpak_write_bits(cmd, cmd_size, 0xf, 4);
        _nqicpak_write_bit(cmd, cmd_size, (s>15)?1:0);
        if (s>15)
            _nqicpak_write_bit(cmd, cmd_size, (s>16)?1:0);
    }
    _nqicpak_write_bits(cmd, cmd_size, e, s + 1);
    _nqicpak_write_bit(cmd, cmd_size, ((n < 0) ? 1 : 0)); //write sign of no
    return 0;
}


/* RLE and diff idx use Elias-gamma packing */

static void _nqicpak_pack_integ(unsigned char* cmd, unsigned int* cmd_size, unsigned int n) {
    uint32_t bb = n;
    uint32_t m;
    uint8_t l = 0;
    if (n == 0)
    {
        printf("number null!\n");
        return;
    }
    while (bb >>= 1) {
        l++;
        _nqicpak_write_bit(cmd, cmd_size, 0);
    }
    for (m = 1 << l; m; m >>= 1) 
        _nqicpak_write_bit(cmd, cmd_size, (n & m) ? 1 : 0);
}

/*
 Encoder for each type (2 bit value):
 -  00 -> RLE+Offset (encoded data directly)
 -  01 -> PrevBlock+Diff+RLE+Offset (ditto, differences previous block)
 -  10 -> OneOffset+Diff+RLE+Offset (encodes a x and y diff of -1 from current position then data)
 -  11 -> LastColOfLastBlockEqual
*/

void _nqicpak_pack_block_4x4_rleoff(nqicpak_block* block, unsigned char* cmd, unsigned int* cmd_size, unsigned char* pel, unsigned int* pel_size, nqi_pelhash* ph, unsigned int base, int *curr_diff)
{
    unsigned int curr_pel = 0;

    int curr_x = 0;
    int curr_y = 0;

    while (curr_pel < 4 * 4)
    {
        int pos;
        unsigned int run;
        bool write_pel = true;
        run = _nqicpak_find_rle_4x4(block, curr_pel);
        _nqicpak_pack_integ(cmd, cmd_size, run);
        {
            int curr_off = nqi_pel_find(ph, pel, base, *pel_size, &block->pels[curr_pel]);
            int diff = ((*pel_size) - curr_off) / 3;
            int diff2 = diff - *curr_diff;
            bool mode = false;
            if (curr_off >= 0 && curr_off <= (0xffffff * 3))
            {
                unsigned int diff2_ns = (diff2 < 0) ? (diff2 * -1) : diff2;
                if (diff2_ns <= 0x5FFFF)
                    mode = true;
            }
            if (mode == true)
            {
                _nqicpak_write_bit(cmd, cmd_size, 0);
                if (diff2 == 0)
                    _nqicpak_write_bit(cmd, cmd_size, 0);
                else
                {
                    _nqicpak_write_bit(cmd, cmd_size, 1);
                    _nqicpak_pack_int4(cmd, cmd_size, diff2);
                }
                *curr_diff = diff;
            }
            else
            {
                _nqicpak_write_bit(cmd, cmd_size, 1);
                memcpy(&pel[(*pel_size)], &block->pels[curr_pel], 3);
                (*pel_size) += 3;
            }
        }
        curr_pel += run;
    }
}

/* Common between type 1 and 2 */

void _nqicpak_pack_block_4x4_prevdiff_rleoff(nqicpak_block* block, nqicpak_block* block_previous, unsigned char* cmd, unsigned int* cmd_size, unsigned char* pel, unsigned int* pel_size, nqi_pelhash* ph, nqi_dtmap* dt, unsigned int base, int *curr_diff)
{
    unsigned int curr_pel = 0;
    nqicpak_rgba diffs[256];
    
    int curr_x = 0;
    int curr_y = 0;

    while (curr_pel < 4 * 4)
    {
        diffs[curr_pel].r = block->pels[curr_pel].r - block_previous->pels[curr_pel].r;
        diffs[curr_pel].g = block->pels[curr_pel].g - block_previous->pels[curr_pel].g;
        diffs[curr_pel].b = block->pels[curr_pel].b - block_previous->pels[curr_pel].b;
        curr_pel++;
    }

    curr_pel = 0;

    while (curr_pel < 4 * 4)
    {
        int pos;
        unsigned int run;
        run = _nqicpak_find_rle_4x4_rgba(diffs, curr_pel);
        _nqicpak_pack_integ(cmd, cmd_size, run);
        {
            int pos_in_tbl = 0;
            unsigned int s = 0, a = 0;
            bool equal = (diffs[curr_pel].r == 0 && diffs[curr_pel].g == 0 && diffs[curr_pel].b == 0);
            _nqicpak_write_bit(cmd, cmd_size, equal);
            if (equal == false)
            {
                bool write_pel = true;
                pos_in_tbl = nqi_dtmap_find(dt, ((diffs[curr_pel].b << 16) | (diffs[curr_pel].g << 8) | (diffs[curr_pel].r)));
                _nqicpak_write_bit(cmd, cmd_size, pos_in_tbl != -1 ? 1 : 0);
                if (pos_in_tbl != -1)
                {
                    _nqicpak_pack_integ(cmd, cmd_size, pos_in_tbl);
                    write_pel = false;
                }
                if (write_pel == true)
                {
                    int curr_off = nqi_pel_find(ph, pel, base, *pel_size, &block->pels[curr_pel]);
                    int diff = ((*pel_size) - curr_off) / 3;
                    int diff2 = diff - *curr_diff;
                    bool found_in_pel = false;
                    if (curr_off >= 0 && curr_off <= (0xffffff * 3))
                    {
                        unsigned int diff2_ns = (diff2 < 0) ? (diff2 * -1) : diff2;
                        if (diff2_ns <= 0x5FFFF)
                            found_in_pel = true;
                    }
                    if (found_in_pel == true)
                    {
                        _nqicpak_write_bit(cmd, cmd_size, 0); 
                        if (diff2 == 0)
                            _nqicpak_write_bit(cmd, cmd_size, 0);
                        else
                        {
                            _nqicpak_write_bit(cmd, cmd_size, 1);
                            _nqicpak_pack_int4(cmd, cmd_size, diff2);
                        }
                        *curr_diff = diff;
                    }
                    else
                    {
                        _nqicpak_write_bit(cmd, cmd_size, 1);
                        memcpy(&pel[(*pel_size)], &block->pels[curr_pel], 3);
                        (*pel_size) += 3;
                    }
                }
            }
        }
        curr_pel += run;
    }
}

/* For type 3 */

bool _nqicpak_pack_block_are_equal(nqicpak_block* block, nqicpak_block* block_previous)
{
    int p = 0;
    int eq = 0;
    int x = 0;
    int y = 0;
    while (y < block->h)
    {
        x = 0;
        while (x < block->w)
        {
            eq+= _nqicpak_equal_pixel_4x4(block->pels[(y * 4) + x], block_previous->pels[(y * 4) + x]);
            x++;
        }
        y++;
    }
    return (eq == block->w * block->h) ? true : false;
}


void nqicpak_context_init(nqicpak_context* nqic, const unsigned char* src, nqic_dim* dim, unsigned int size)
{
    nqicpak_context_internal* temp_ctxt;
    bool ret = false;

    temp_ctxt = (nqicpak_context_internal*)malloc(sizeof(nqicpak_context_internal));

    if (temp_ctxt == NULL)
        goto clean_me;

    memset(temp_ctxt, 0, sizeof(nqicpak_context_internal));

    if (src == NULL || dim == NULL || size == 0)
        goto clean_me;

    if (dim->width <= 0 || dim->height <= 0)
        goto clean_me;

    temp_ctxt->src = src;
    temp_ctxt->src_dim = dim;
    temp_ctxt->src_size = size;

    //prepare our image
    {
        unsigned int align_width = (temp_ctxt->src_dim->width + 3) / 4;
        unsigned int align_height = (temp_ctxt->src_dim->height + 3) / 4;
        unsigned int current_block = 0;
        temp_ctxt->blocks = (nqicpak_block**)malloc(sizeof(nqicpak_block*) * (align_width * align_height));
        if (temp_ctxt->blocks == NULL)
            goto clean_me;
        memset(temp_ctxt->blocks, 0, sizeof(nqicpak_block*) * (align_width * align_height));
        temp_ctxt->blocks_cnt = align_width * align_height;
        while (current_block < temp_ctxt->blocks_cnt)
        {
            nqicpak_block* curr_block;
            int x = (current_block % align_width) * 4;
            int y = (current_block / align_width) * 4;
            int c_x = 0, c_y = 0;
            curr_block = (nqicpak_block*)malloc(sizeof(nqicpak_block));
            if (curr_block == NULL)
                goto clean_me;
            memset(curr_block, 0, sizeof(nqicpak_block));
            _nqicpak_read_block_4x4(curr_block, temp_ctxt->src, temp_ctxt->src_dim, x, y);
            temp_ctxt->blocks[current_block] = curr_block;
            current_block++;
        }
    }

    //prepare datas
    temp_ctxt->cmd = (unsigned char*)malloc(size*2);
    if (temp_ctxt->cmd == NULL)
        goto clean_me;
    memset(temp_ctxt->cmd, 0, size*2);
    temp_ctxt->cmd_size = 0;
    temp_ctxt->pels = (unsigned char*)malloc(temp_ctxt->blocks_cnt * 48);
    if (temp_ctxt->pels == NULL)
        goto clean_me;
    memset(temp_ctxt->pels, 0, temp_ctxt->blocks_cnt * 48);
    temp_ctxt->pels_size = 0;

    nqi_pelhash_init(&temp_ctxt->ph, temp_ctxt->blocks_cnt * 16);
    nqi_dtmap_init(&temp_ctxt->dt, nqic_diff_tbl, NQI_IDX_MAX + 1);

    ret = true;
    *nqic = (nqicpak_context)temp_ctxt;

clean_me:
    if (ret == false)
    {
        if (temp_ctxt != NULL)
        {
            if (temp_ctxt->blocks)
            {
                unsigned int curr_block = 0;
                while (curr_block < temp_ctxt->blocks_cnt)
                {
                    if (temp_ctxt->blocks[curr_block] != NULL)
                        free(temp_ctxt->blocks[curr_block]);
                    curr_block++;
                }
                free(temp_ctxt->blocks);
            }
            if (temp_ctxt->cmd)
                free(temp_ctxt->cmd);
            if (temp_ctxt->pels)
                free(temp_ctxt->pels);
            free(temp_ctxt);
        }
        *nqic = NULL;
    }

    return;
}

bool nqicpak_encode(nqicpak_context nqic)
{
    nqicpak_context_internal* temp_ctxt;
    unsigned int curr_block = 0;
    bool restart_like_new = false;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqicpak_context_internal*)nqic;

    memset(temp_ctxt->pels, 0, temp_ctxt->blocks_cnt*48);
    temp_ctxt->pels_size = 0;

    nqi_pelhash_reset(&temp_ctxt->ph);

    while (curr_block < temp_ctxt->blocks_cnt)
    {
        bool row_duplicate = false;
        unsigned int align_row_blocks = (temp_ctxt->src_dim->width / 4) + (temp_ctxt->src_dim->width % 4 ? 1 : 0);
        unsigned char costs[4][64];
        unsigned char costs_pels[4][48];
        unsigned int costs_sizes[4];
        unsigned int costs_pels_sizes[4];
        unsigned int costs_totals_sizes[4];
        int curr_diffs[4];
        unsigned int cost_count = 1;
        unsigned int i;
        unsigned int curr_candidate_block = 0;
        unsigned int pels_prev_size = temp_ctxt->pels_size;
        bool force_type3 = false;
        nqicpak_block my_block;
        int best_type = 0;
        //We do row checking in this step...
        if (curr_block >= align_row_blocks)
        {
            unsigned int equal_row_blocks = 0;

            unsigned int curr_row = curr_block / align_row_blocks;
            unsigned int prev_row = (curr_block - 1) / align_row_blocks;

            if (curr_row != prev_row && curr_row > 0)
            {
                unsigned int row_start = curr_row * align_row_blocks;
                unsigned int prev_row_start = prev_row * align_row_blocks;
                unsigned int i;

                for (i = 0; i < align_row_blocks; i++)
                {
                    if (row_start + i < temp_ctxt->blocks_cnt &&
                        prev_row_start + i < temp_ctxt->blocks_cnt)
                    {
                        if (_nqicpak_pack_block_are_equal(
                            temp_ctxt->blocks[row_start + i],
                            temp_ctxt->blocks[prev_row_start + i]))
                        {
                            equal_row_blocks++;
                        }
                    }
                }
                row_duplicate = (equal_row_blocks == align_row_blocks);
            }
        }

        if (curr_block % align_row_blocks == 0)
            _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, (row_duplicate == true) ? 1 : 0); //flag for each row, is it same as previous or not?
        if (row_duplicate == false) //not the same as last row, so encoding block by block
        {
            unsigned int matches = 1;
            costs_sizes[0] = 0;
            costs_sizes[1] = 0;
            costs_sizes[2] = 0;
            costs_sizes[3] = 0;
            costs_pels_sizes[0] = 0;
            costs_pels_sizes[1] = 0;
            costs_pels_sizes[2] = 0;
            costs_pels_sizes[3] = 0;
            costs_totals_sizes[0] = 0;
            costs_totals_sizes[1] = 0;
            costs_totals_sizes[2] = 0;
            costs_totals_sizes[3] = 2; //this will change to 4096 is type3 candidate is impossible
            curr_diffs[0] = 0;
            curr_diffs[1] = 0;
            curr_diffs[2] = 0;
            curr_diffs[3] = -1; //no use

            if (curr_block)
            {
                if (_nqicpak_pack_block_are_equal(temp_ctxt->blocks[curr_block], temp_ctxt->blocks[curr_block - 1]) == true)
                    force_type3 = true;
            }
            if (force_type3 == true)
            {
                _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, 1);
                //check if more blocks are literally the same
                curr_block++;
                while (curr_block % align_row_blocks != 0 && curr_block < temp_ctxt->blocks_cnt && matches < align_row_blocks)
                {
                    if (_nqicpak_pack_block_are_equal(temp_ctxt->blocks[curr_block], temp_ctxt->blocks[curr_block - 1]) == true)
                        matches++, curr_block++;
                    else
                        break;
                }
                curr_block--;
                printf("block %d->%d are equal\n", curr_block-matches+1, curr_block);
                _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, (matches>1)?1:0);
                if (matches > 1)
                    _nqicpak_pack_integ(temp_ctxt->cmd, &temp_ctxt->cmd_size, matches);
            }
            else
            {
                //Just RLE'd block
                int prev_curr_diff = temp_ctxt->curr_diff;
                _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, 0);
                _nqicpak_write_bit(costs[0], &costs_sizes[0], 0);
                _nqicpak_write_bit(costs[0], &costs_sizes[0], 0);
                _nqicpak_pack_block_4x4_rleoff(temp_ctxt->blocks[curr_block], costs[0], &costs_sizes[0], temp_ctxt->pels, &temp_ctxt->pels_size, &temp_ctxt->ph, pels_prev_size, &temp_ctxt->curr_diff);
                costs_pels_sizes[0] = temp_ctxt->pels_size - pels_prev_size;
                memcpy(costs_pels[0], temp_ctxt->pels + pels_prev_size, costs_pels_sizes[0]);
                temp_ctxt->pels_size = pels_prev_size;
                nqi_pelhash_rollback(&temp_ctxt->ph, temp_ctxt->pels, (pels_prev_size / 3));
                curr_diffs[0] = temp_ctxt->curr_diff;
                temp_ctxt->curr_diff = prev_curr_diff;
                costs_totals_sizes[0] = costs_sizes[0] + (8*costs_pels_sizes[0]);
                if (curr_block)
                {
                    bool stop = false;
                    int offsets[3][2] = { {-1,0}, {0,-1}, {-1,-1} };
                    int curr_x = (curr_block % align_row_blocks) * 4;
                    int curr_y = (curr_block / align_row_blocks) * 4;
                    int what_offset = 0;
                    //RLE'd+diff'd to previous block
                    if (curr_block % align_row_blocks != 0)
                    {
                        _nqicpak_write_bit(costs[1], &costs_sizes[1], 1);
                        _nqicpak_write_bit(costs[1], &costs_sizes[1], 0);
                        _nqicpak_pack_block_4x4_prevdiff_rleoff(temp_ctxt->blocks[curr_block], temp_ctxt->blocks[curr_block - 1], costs[1], &costs_sizes[1], temp_ctxt->pels, &temp_ctxt->pels_size, &temp_ctxt->ph, &temp_ctxt->dt, pels_prev_size, &temp_ctxt->curr_diff);
                        costs_pels_sizes[1] = temp_ctxt->pels_size - pels_prev_size;
                        memcpy(costs_pels[1], temp_ctxt->pels + pels_prev_size, costs_pels_sizes[1]);
                        temp_ctxt->pels_size = pels_prev_size;
                        nqi_pelhash_rollback(&temp_ctxt->ph, temp_ctxt->pels, (pels_prev_size / 3));
                        curr_diffs[1] = temp_ctxt->curr_diff;
                        temp_ctxt->curr_diff = prev_curr_diff;
                        costs_totals_sizes[1] = costs_sizes[1] + (8*costs_pels_sizes[1]);
                        memcpy(costs_pels[1], temp_ctxt->pels + temp_ctxt->pels_size, temp_ctxt->pels_size - pels_prev_size);
                    }
                    else
                        costs_totals_sizes[1] = 4096; //cannot use prevblock diff on new row
                    cost_count = 4;
                    
                    while (what_offset < 3)
                    {
                        //RLE'd+diff'd block at non-prev block (i.e. block at curr x - 1 or curr y - 1 ...)
                        unsigned char candidate_cost[64];
                        unsigned int candidate_size = 0;
                        unsigned char candidate_pels[48];
                        unsigned int candidate_pels_size = 0;
                        unsigned int candidate_total_size = 0;
                        int candidate_diff = 0;
                        int cx = curr_x + offsets[what_offset][0];
                        int cy = curr_y + offsets[what_offset][1];
                        bool offs_bk_equals = false;
                        if (cx < 0 || cy < 0)
                        {
                            what_offset++; //skip!
                            continue;
                        }
                        _nqicpak_write_bit(candidate_cost, &candidate_size, 0);
                        _nqicpak_write_bit(candidate_cost, &candidate_size, 1);
                        _nqicpak_write_bits(candidate_cost, &candidate_size, what_offset, 2);
                        _nqicpak_read_block_4x4(&my_block, temp_ctxt->src, temp_ctxt->src_dim, curr_x + offsets[what_offset][0], curr_y + offsets[what_offset][1]);
                        _nqicpak_pack_block_4x4_prevdiff_rleoff(temp_ctxt->blocks[curr_block], &my_block, candidate_cost, &candidate_size, temp_ctxt->pels, &temp_ctxt->pels_size, &temp_ctxt->ph, &temp_ctxt->dt, pels_prev_size, &temp_ctxt->curr_diff);
                        candidate_pels_size = temp_ctxt->pels_size - pels_prev_size;
                        memcpy(candidate_pels, temp_ctxt->pels + pels_prev_size, candidate_pels_size);
                        temp_ctxt->pels_size = pels_prev_size;
                        nqi_pelhash_rollback(&temp_ctxt->ph, temp_ctxt->pels, (pels_prev_size / 3));
                        candidate_diff = temp_ctxt->curr_diff;
                        temp_ctxt->curr_diff = prev_curr_diff;
                        candidate_total_size = candidate_size + (8 * candidate_pels_size);
                        if (candidate_total_size < costs_totals_sizes[2] || costs_totals_sizes[2] == 0)
                        {
                            memcpy(costs[2], candidate_cost, (candidate_size + 7) / 8);
                            memcpy(costs_pels[2], candidate_pels, candidate_pels_size);
                            costs_sizes[2] = candidate_size;
                            costs_pels_sizes[2] = candidate_pels_size;
                            costs_totals_sizes[2] = candidate_total_size;
                            curr_diffs[2] = candidate_diff;
                        }
                        what_offset++;
                    }


                    //for type 3 we check if previous block STRICTLY is the same as previous col for all other cols...
                    //4096 is used as a skip flag
                    {
                        unsigned char candidate_cost[64];
                        unsigned int candidate_size = 0;
                        unsigned char candidate_pels[48];
                        unsigned int candidate_pels_size = 0;
                        unsigned int candidate_total_size = 0;
                        int cx = curr_x - 1;
                        bool offs_bk_equals = false;
                        _nqicpak_write_bit(costs[3], &costs_sizes[3], 1);
                        _nqicpak_write_bit(costs[3], &costs_sizes[3], 1);
                        if (cx < 0)
                        {
                            costs_totals_sizes[3] = 4096;
                        }
                        else
                        {
                            _nqicpak_read_block_4x4(&my_block, temp_ctxt->src, temp_ctxt->src_dim, cx, curr_y);
                            if (_nqicpak_pack_block_are_equal(temp_ctxt->blocks[curr_block], &my_block) == false)
                                costs_totals_sizes[3] = 4096;
                        }
                    }
                }

                {
                    //save some bit of time by choosing best type instead of sort
                    unsigned int best_total = 4096; //flag
                    for (i = 0; i < cost_count; i++)
                    {
                        if (costs_totals_sizes[i] < best_total && costs_totals_sizes[i]>0)
                        {
                            best_total = costs_totals_sizes[i];
                            best_type = i;
                        }
                    }
                    printf("\rblock %d: %06d %06d %06d %06d best->%d\n", curr_block, costs_totals_sizes[0], costs_totals_sizes[1], costs_totals_sizes[2], costs_totals_sizes[3], best_type);
                    for (i = 0; i < costs_sizes[best_type] / 8; i++)
                        _nqicpak_write_bits(temp_ctxt->cmd, &temp_ctxt->cmd_size, costs[best_type][i], 8);
                    for (i = 0; i < (costs_sizes[best_type] % 8); i++)
                        _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, costs[best_type][costs_sizes[best_type] / 8] >> (7 - i));
                }
                if (best_type != 3)
                {
                    memcpy(temp_ctxt->pels + temp_ctxt->pels_size, costs_pels[best_type], costs_pels_sizes[best_type]);
                    temp_ctxt->pels_size += costs_pels_sizes[best_type];
                    nqi_pelhash_sync(&temp_ctxt->ph, temp_ctxt->pels, temp_ctxt->pels_size);
                    temp_ctxt->curr_diff = curr_diffs[best_type];
                }
            }
            if (temp_ctxt->blocks[curr_block]->w * temp_ctxt->blocks[curr_block]->h != 4 * 4)
                restart_like_new = true;
            else
                if (restart_like_new == true) restart_like_new = false;
            curr_block++;
        }
        else
            curr_block += align_row_blocks;
    }
    ret = true;

clean_me:
    return ret;
}

bool nqicpak_flush(nqicpak_context nqic)
{
    nqicpak_context_internal* temp_ctxt;
    unsigned int curr_block = 0;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqicpak_context_internal*)nqic;

    printf("\rtotal sz: %d\n", (temp_ctxt->cmd_size + 7) / 8 + temp_ctxt->pels_size);

    ret = true;

clean_me:
    return ret;
}
