#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "nqicpak.h"
#include "nqichash.h"

#ifdef NQI_STATS
typedef struct { unsigned char k; int v; int v2; int v3; } nqi_tok;
typedef struct { nqi_tok t[200]; unsigned n; } nqi_cand;
static nqi_cand g_cur, g_cand[4];
static nqi_tok* g_log; static size_t g_logn, g_logcap;
static unsigned long g_blk[5], g_candbits;
#define TOK4(kk,vv,vv2,vv3) do { g_cur.t[g_cur.n].k=(kk); g_cur.t[g_cur.n].v=(vv); g_cur.t[g_cur.n].v2=(vv2); g_cur.t[g_cur.n].v3=(vv3); g_cur.n++; } while(0)
#define TOK(kk,vv,vv2) TOK4(kk,vv,vv2,0)
#define NQI_PACK(p) (((int)(p).r<<16)|((int)(p).g<<8)|(int)(p).b)
static void g_commit(const nqi_cand* c, int type, unsigned cbits)
{
    unsigned i;
    if (g_logn + c->n + 1 > g_logcap) { g_logcap = g_logcap ? g_logcap*2 : (1u<<20); g_log = (nqi_tok*)realloc(g_log, g_logcap*sizeof(nqi_tok)); }
    g_log[g_logn].k = 'B'; g_log[g_logn].v = type; g_logn++;
    for (i = 0; i < c->n; i++) g_log[g_logn++] = c->t[i];
}
#else
#define TOK(kk,vv,vv2) ((void)0)
#define TOK4(kk,vv,vv2,vv3) ((void)0)
#define NQI_PACK(p) 0
#endif

/* src: https://stackoverflow.com/a/20468180 */

#define nqicpak_log         //printf

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

#ifdef NQI_STATS
#include "nqicanalyze.h"
#endif

extern int nqic_diff_tbl[]; /* nqid.c has this table */

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} nqicpak_rgba;

typedef struct {
    nqicpak_rgba pels[16];
    unsigned int w;
    unsigned int h;
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

    //Output
    unsigned char* out_buffer;
    unsigned int out_size;

    bool flushed;
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
            while (c_x < 4)
            {
                nqicpak_rgba* pelp;
                if (c_x + x >= dim->width)
                    pelp = &block->pels[(c_y-1) * 4 + (c_x-1)];
                else
                    pelp = &block->pels[(c_y-1) * 4 + c_x];
                memcpy(&block->pels[c_y * 4 + c_x], pelp, sizeof(nqicpak_rgba));
                c_x++;
            }
            block->h--;
        }
        else
        {
            while (c_x < 4)
            {
                nqicpak_rgba* pelp;
                if (c_x + x >= dim->width)
                    pelp = &block->pels[c_y * 4 + (c_x-1)];
                else
                    pelp = (nqicpak_rgba*)(block_start + ((c_y * dim->width + c_x) * 4));
                memcpy(&block->pels[c_y * 4 + c_x], pelp, sizeof(nqicpak_rgba));
                if (c_x + x >= dim->width && c_y == 0) block->w--;
                c_x++;
            }
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

#define NQI_IDX_MAX 511

/*
Offset idx uses 4bit+rem integer encoding 
For 15+ bits, we add some more bits to save costs a bit... If < 15 bits, then no need
*/

static int nqi_idx_split4(uint32_t index, unsigned* shift, uint32_t* extra)
{
    if (index > 0x5FFFE)
        return -1;

    unsigned s = (31u - (unsigned)__builtin_clz(index + 2u)) - 1u;

    *shift = s;
    *extra = index - nqi_idx_base(s);
    return 0;
}

static int _nqicpak_pack_int4(unsigned char* cmd, unsigned int* cmd_size, int n)
{
    unsigned s; uint32_t e;
    unsigned int index_unsigned = abs(n);
    nqicpak_log("int4: i am writing %d\n", n);
    if (nqi_idx_split4(index_unsigned, &s, &e))
        return -1;
    if (s < 8)
    {
        _nqicpak_write_bit(cmd, cmd_size, 0);
        _nqicpak_write_bits(cmd, cmd_size, index_unsigned, 9);
    }
    else
    {
        _nqicpak_write_bit(cmd, cmd_size, 1);
        if (s < 15)
            _nqicpak_write_bits(cmd, cmd_size, s-8, 3);
        else
        {
            _nqicpak_write_bits(cmd, cmd_size, 0x7, 3);
            _nqicpak_write_bit(cmd, cmd_size, (s > 15) ? 1 : 0);
            if (s > 15)
                _nqicpak_write_bit(cmd, cmd_size, (s > 16) ? 1 : 0);
        }
        _nqicpak_write_bits(cmd, cmd_size, e, s + 1);
    }
    _nqicpak_write_bit(cmd, cmd_size, ((n < 0) ? 1 : 0)); //write sign of no
    return 0;
}


/* RLE and diff idx use Elias-gamma packing */

static void _nqicpak_pack_integ(unsigned char* cmd, unsigned int* cmd_size, unsigned int n) {
    int nb = 0;
    nqicpak_log("eg: i am writing %d\n", n);
    for (uint32_t t = n; t; t >>= 1)
        nb++;
    _nqicpak_write_bit(cmd, cmd_size, (n>1)?1:0);
    for (int i = nb - 2; i >= 0; i--) {
        _nqicpak_write_bit(cmd, cmd_size, (int)((n >> i) & 1));
        _nqicpak_write_bit(cmd, cmd_size, i == 0);
    }
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
#ifdef NQI_STATS
    g_cur.n = 0;
#endif
    unsigned int run;

    while (curr_pel < 4 * 4)
    {
        run = _nqicpak_find_rle_4x4(block, curr_pel); TOK('R', run, 0);
        _nqicpak_pack_integ(cmd, cmd_size, run);
        {
            int curr_off = nqi_pel_find(ph, pel, base, *pel_size, &block->pels[curr_pel]);
            int diff = ((*pel_size) - curr_off) / 3;
            int diff2 = diff - *curr_diff;
            bool valid_to_exist_in_prev = false;
            if (curr_off >= 0 && curr_off <= (0xffffff * 3))
            {
                if (abs(diff2) <= 0x5FFFE)
                    valid_to_exist_in_prev = true;
            }
            if (valid_to_exist_in_prev == true)
            {
                _nqicpak_write_bit(cmd, cmd_size, 0);
                nqicpak_log("pel in prev\n");
                _nqicpak_pack_int4(cmd, cmd_size, diff2);
                TOK('D', diff2, curr_off/3);
                *curr_diff = diff;
            }
            else
            {
                _nqicpak_write_bit(cmd, cmd_size, 1);
                nqicpak_log("pel not in prev\n");
                TOK4('W', NQI_PACK(block->pels[curr_pel]), (*pel_size)/3, curr_pel ? NQI_PACK(block->pels[curr_pel-1]) : -1);
                memcpy(&pel[(*pel_size)], &block->pels[curr_pel], 3);
                (*pel_size) += 3;
                nqi_pelhash_sync(ph, pel, *pel_size);
            }
        }
        curr_pel += run;
    }
}

/* Common between type 1 and 2 */

void _nqicpak_pack_block_4x4_prevdiff_rleoff(nqicpak_block* block, nqicpak_block* block_previous, unsigned char* cmd, unsigned int* cmd_size, unsigned char* pel, unsigned int* pel_size, nqi_pelhash* ph, nqi_dtmap* dt, unsigned int base, int *curr_diff)
{
    unsigned int curr_pel = 0;
#ifdef NQI_STATS
    g_cur.n = 0;
#endif
    nqicpak_rgba diffs[16];
    unsigned int run;
    int pos_in_tbl;

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
        run = _nqicpak_find_rle_4x4_rgba(diffs, curr_pel); TOK('R', run, 0);
        _nqicpak_pack_integ(cmd, cmd_size, run);
        {
            bool equal = (diffs[curr_pel].r == 0 && diffs[curr_pel].g == 0 && diffs[curr_pel].b == 0);
            _nqicpak_write_bit(cmd, cmd_size, equal); if (equal) TOK('E', 0, 0);
            if (equal == false)
            {
                bool write_pel = true;
                nqicpak_log("pel not skipped\n");
                pos_in_tbl = nqi_dtmap_find(dt, ((diffs[curr_pel].b << 16) | (diffs[curr_pel].g << 8) | (diffs[curr_pel].r)));
                _nqicpak_write_bit(cmd, cmd_size, pos_in_tbl != -1 ? 1 : 0);
                if (pos_in_tbl != -1)
                {
                    nqicpak_log("pel is diff\n");
                    _nqicpak_pack_integ(cmd, cmd_size, pos_in_tbl); TOK('T', pos_in_tbl, 0);
                    write_pel = false;
                }
                if (write_pel == true)
                {
                    int curr_off = nqi_pel_find(ph, pel, base, *pel_size, &block->pels[curr_pel]);
                    int diff = ((*pel_size) - curr_off) / 3;
                    int diff2 = diff - *curr_diff;
                    bool valid_to_exist_in_prev = false;
                    nqicpak_log("pel not diff\n");
                    if (curr_off >= 0 && curr_off <= (0xffffff * 3))
                    {
                        if (abs(diff2) <= 0x5FFFE)
                            valid_to_exist_in_prev = true;
                    }
                    if (valid_to_exist_in_prev == true)
                    {
                        _nqicpak_write_bit(cmd, cmd_size, 0);
                        nqicpak_log("pel in prev\n");
                        _nqicpak_pack_int4(cmd, cmd_size, diff2); TOK('D', diff2, curr_off/3);
                        *curr_diff = diff;
                    }
                    else
                    {
                        _nqicpak_write_bit(cmd, cmd_size, 1);
                        nqicpak_log("pel not in prev\n");
                        TOK4('W', NQI_PACK(block->pels[curr_pel]), (*pel_size)/3, NQI_PACK(block_previous->pels[curr_pel]));
                        memcpy(&pel[(*pel_size)], &block->pels[curr_pel], 3);
                        (*pel_size) += 3;
                        nqi_pelhash_sync(ph, pel, *pel_size);
                    }
                }
            }
            else
                nqicpak_log("pel skipped\n");
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
            if (temp_ctxt->blocks != NULL)
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
        unsigned int align_row_blocks = ((temp_ctxt->src_dim->width+3) / 4);
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
            costs_totals_sizes[3] = 2; //this will change to 4096 if type3 candidate is impossible
            curr_diffs[0] = 0;
            curr_diffs[1] = 0;
            curr_diffs[2] = 0;
            curr_diffs[3] = -1; //no use

            if (curr_block % align_row_blocks != 0)
            {
                if (_nqicpak_pack_block_are_equal(temp_ctxt->blocks[curr_block], temp_ctxt->blocks[curr_block - 1]) == true)
                    force_type3 = true;
            }
            if (force_type3 == true)
            {
                nqicpak_log("duplicate\n");
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
                _nqicpak_pack_integ(temp_ctxt->cmd, &temp_ctxt->cmd_size, matches);
            }
            else
            {
                //Just RLE'd block
                nqicpak_log("not duplicate\n");
                int prev_curr_diff = temp_ctxt->curr_diff;
                _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, 0);
                _nqicpak_write_bits(costs[0], &costs_sizes[0], 0, 2);
                _nqicpak_pack_block_4x4_rleoff(temp_ctxt->blocks[curr_block], costs[0], &costs_sizes[0], temp_ctxt->pels, &temp_ctxt->pels_size, &temp_ctxt->ph, pels_prev_size, &temp_ctxt->curr_diff);
                costs_pels_sizes[0] = temp_ctxt->pels_size - pels_prev_size;
                memcpy(costs_pels[0], temp_ctxt->pels + pels_prev_size, costs_pels_sizes[0]);
                temp_ctxt->pels_size = pels_prev_size;
                nqi_pelhash_rollback(&temp_ctxt->ph, temp_ctxt->pels, (pels_prev_size / 3));
                curr_diffs[0] = temp_ctxt->curr_diff;
#ifdef NQI_STATS
                g_cand[0] = g_cur;
#endif
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
                        _nqicpak_write_bits(costs[1], &costs_sizes[1], 1, 2);
                        _nqicpak_pack_block_4x4_prevdiff_rleoff(temp_ctxt->blocks[curr_block], temp_ctxt->blocks[curr_block - 1], costs[1], &costs_sizes[1], temp_ctxt->pels, &temp_ctxt->pels_size, &temp_ctxt->ph, &temp_ctxt->dt, pels_prev_size, &temp_ctxt->curr_diff);
                        costs_pels_sizes[1] = temp_ctxt->pels_size - pels_prev_size;
                        memcpy(costs_pels[1], temp_ctxt->pels + pels_prev_size, costs_pels_sizes[1]);
                        temp_ctxt->pels_size = pels_prev_size;
                        nqi_pelhash_rollback(&temp_ctxt->ph, temp_ctxt->pels, (pels_prev_size / 3));
                        curr_diffs[1] = temp_ctxt->curr_diff;
#ifdef NQI_STATS
                        g_cand[1] = g_cur;
#endif
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
                        _nqicpak_write_bits(candidate_cost, &candidate_size, 2, 2);
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
#ifdef NQI_STATS
                            g_cand[2] = g_cur;
#endif
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
                        _nqicpak_write_bits(costs[3], &costs_sizes[3], 3, 2);
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
#ifdef NQI_STATS
                    g_blk[best_type]++; g_candbits += costs_sizes[best_type];
#endif
                    for (i = 0; i < costs_sizes[best_type] / 8; i++)
                        _nqicpak_write_bits(temp_ctxt->cmd, &temp_ctxt->cmd_size, costs[best_type][i], 8);
                    for (i = 0; i < (costs_sizes[best_type] % 8); i++)
                        _nqicpak_write_bit(temp_ctxt->cmd, &temp_ctxt->cmd_size, costs[best_type][costs_sizes[best_type] / 8] >> (7 - i));
                }
                nqicpak_log("best type: %d\n", best_type);
                if (best_type != 3)
                {
                    memcpy(temp_ctxt->pels + temp_ctxt->pels_size, costs_pels[best_type], costs_pels_sizes[best_type]);
                    temp_ctxt->pels_size += costs_pels_sizes[best_type];
                    temp_ctxt->curr_diff = curr_diffs[best_type];
                    nqi_pelhash_sync(&temp_ctxt->ph, temp_ctxt->pels, temp_ctxt->pels_size);
#ifdef NQI_STATS
                    g_commit(&g_cand[best_type], best_type, costs_sizes[best_type]);
#endif
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
    unsigned int cmd_size;
    unsigned int cmd_size_a;
    unsigned int pel_size;
    unsigned int pel_size_a;
    unsigned int pel_count;
    bool ret = false;

    if (nqic == NULL)
    {
        goto clean_me;
    }

    temp_ctxt = (nqicpak_context_internal*)nqic;

    if (temp_ctxt->cmd_size == 0 || temp_ctxt->pels_size == 0)
    {
        goto clean_me;
    }

    cmd_size = (temp_ctxt->cmd_size + 7) / 8;
    cmd_size_a = ((cmd_size + 3) / 4) * 4; //align to 4bytes

    pel_size = temp_ctxt->pels_size;
    pel_size_a = ((pel_size + 3) / 4) * 4; //ditto

    temp_ctxt->out_buffer = (unsigned char*)malloc(8 + cmd_size_a + pel_size_a);
    if (temp_ctxt->out_buffer == NULL)
    {
        goto clean_me;
    }

    temp_ctxt->out_size = 8 + cmd_size_a + pel_size_a;

    memset(temp_ctxt->out_buffer, 0, temp_ctxt->out_size);

    temp_ctxt->out_buffer[0] = cmd_size_a & 0xff;
    temp_ctxt->out_buffer[1] = (cmd_size_a >> 8) & 0xff;
    temp_ctxt->out_buffer[2] = (cmd_size_a >> 16) & 0xff;
    temp_ctxt->out_buffer[3] = (cmd_size_a >> 24) & 0xff;

    memcpy(&temp_ctxt->out_buffer[4], temp_ctxt->cmd, (temp_ctxt->cmd_size + 7) / 8);

    pel_count = pel_size / 3;

    temp_ctxt->out_buffer[4+cmd_size_a] = pel_count & 0xff;
    temp_ctxt->out_buffer[5+cmd_size_a] = (pel_count >> 8) & 0xff;
    temp_ctxt->out_buffer[6+cmd_size_a] = (pel_count >> 16) & 0xff;
    temp_ctxt->out_buffer[7+cmd_size_a] = (pel_count >> 24) & 0xff;

    memcpy(&temp_ctxt->out_buffer[8+cmd_size_a], temp_ctxt->pels, pel_size);

#ifdef NQI_STATS
    nqi_analyze(stdout, temp_ctxt->cmd_size);
#endif

    free(temp_ctxt->cmd);
    temp_ctxt->cmd = NULL;
    temp_ctxt->cmd_size = 0;

    free(temp_ctxt->pels);
    temp_ctxt->pels = NULL;
    temp_ctxt->pels_size = 0;

    ret = true;

    temp_ctxt->flushed = true;

clean_me:
    return ret;
}

bool nqicpak_get_output(nqicpak_context nqic, unsigned char** data, unsigned int* size)
{
    nqicpak_context_internal* temp_ctxt;
    if (nqic == NULL)
    {
        *data = NULL;
        *size = 0;
        return false;
    }

    temp_ctxt = (nqicpak_context_internal*)nqic;

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

void nqicpak_free(nqicpak_context* nqic)
{
    nqicpak_context_internal* temp_ctxt;
    if (*nqic != NULL)
    {
        temp_ctxt = *nqic;
        if (temp_ctxt->blocks != NULL)
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
        if (temp_ctxt->out_buffer)
            free(temp_ctxt->out_buffer);
        free(temp_ctxt);
        *nqic = NULL;
    }
}