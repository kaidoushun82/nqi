#ifndef _NQICHASH_H_
#define _NQICHASH_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline unsigned nqi_hash24(uint32_t v, unsigned bits)
{
    return (unsigned)((v* 2654435761u) >> (32 - bits));
}

/* diff hashing */

#define NQI_DT_BITS 10
#define NQI_DT_SIZE (1u << NQI_DT_BITS)

typedef struct {
    const int* tbl;
    int16_t slot[NQI_DT_SIZE];          /* table index + 1, 0 = empty */
} nqi_dtmap;

static void nqi_dtmap_init(nqi_dtmap* m, const int* tbl, unsigned count)
{
    unsigned i;
    m->tbl = tbl;
    memset(m->slot, 0, sizeof(m->slot));
    for (i = 0; i < count; i++)
    {
        uint32_t key = (uint32_t)tbl[i] & 0xFFFFFFu;
        unsigned h = nqi_hash24(key, NQI_DT_BITS);
        while (m->slot[h] && ((uint32_t)tbl[m->slot[h] - 1] & 0xFFFFFFu) != key)
            h = (h + 1) & (NQI_DT_SIZE - 1);
        if (!m->slot[h])
            m->slot[h] = (int16_t)(i + 1);
    }
}

static inline int nqi_dtmap_find(const nqi_dtmap* m, uint32_t key)
{
    unsigned h = nqi_hash24(key, NQI_DT_BITS);
    int s;
    while ((s = m->slot[h]) != 0)
    {
        if (((uint32_t)m->tbl[s - 1] & 0xFFFFFFu) == key)
            return s - 1;
        h = (h + 1) & (NQI_DT_SIZE - 1);
    }
    return -1;
}

/* pixel hist hashing */

#define NQI_PH_BITS     18
#define NQI_PH_WINDOW   0xffffff

typedef struct {
    int32_t* head;
    int32_t* prev;
    unsigned committed;
} nqi_pelhash;

static inline uint32_t nqi_rgb_key(const unsigned char* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static void nqi_pelhash_reset(nqi_pelhash* ph)
{
    memset(ph->head, 0xff, sizeof(int32_t) << NQI_PH_BITS);
    ph->committed = 0;
}

static int nqi_pelhash_init(nqi_pelhash* ph, unsigned max_entries)
{
    ph->head = (int32_t*)malloc(sizeof(int32_t) << NQI_PH_BITS);
    ph->prev = (int32_t*)malloc(sizeof(int32_t) * (max_entries ? max_entries : 1));
    if (!ph->head || !ph->prev)
    {
        free(ph->head);
        free(ph->prev);
        ph->head = ph->prev = NULL;
        return 0;
    }
    nqi_pelhash_reset(ph);
    return 1;
}

static void nqi_pelhash_free(nqi_pelhash* ph)
{
    free(ph->head);
    free(ph->prev);
    ph->head = ph->prev = NULL;
}

static void nqi_pelhash_sync(nqi_pelhash* ph, const unsigned char* pel, unsigned pel_size)
{
    unsigned n = pel_size / 3;
    while (ph->committed < n)
    {
        unsigned e = ph->committed;
        unsigned h = nqi_hash24(nqi_rgb_key(pel + e * 3), NQI_PH_BITS);
        ph->prev[e] = ph->head[h];
        ph->head[h] = (int32_t)e;
        ph->committed++;
    }
}

static void nqi_pelhash_rollback(nqi_pelhash* ph, const unsigned char* pel, unsigned old_committed)
{
    while (ph->committed > old_committed)
    {
        unsigned e = --ph->committed;
        unsigned h = nqi_hash24(nqi_rgb_key(pel + e * 3),NQI_PH_BITS);
        ph->head[h] = ph->prev[e];
    }
}


static inline int nqi_pel_find(const nqi_pelhash* ph, const unsigned char* pel,
                               unsigned base, unsigned pel_size,
                               const unsigned char* rgb)
{
    int off;

    for (off = (int)pel_size - 3; off >= (int)base; off -= 3)
        if (pel[off] == rgb[0] && pel[off + 1] == rgb[1] && pel[off + 2] == rgb[2])
            return off;

    if (ph)
    {
        int e = ph->head[nqi_hash24(nqi_rgb_key(rgb), NQI_PH_BITS)];
        while (e >= 0)
        {
            off = e * 3;
            if ((pel_size - (unsigned)off) / 3 > NQI_PH_WINDOW)
                break;
            if (pel[off] == rgb[0] && pel[off + 1] == rgb[1] && pel[off + 2] == rgb[2])
                return off;
            e = ph->prev[e];
        }
    }
    return -1;
}

#endif