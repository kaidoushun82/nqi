/* !!! TO BE INCLUDED IN NQICPAK.C !!! */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#ifndef _NQIANALYZE_H_
#define _NQIANALYZE_H_
static int nqi_gam(unsigned n)
{
    int l = 0;
    while (n >>= 1)
        l++;
    return 2 * l + 1;
}
static int nqi_bucket(unsigned u)
{
    return (31 - __builtin_clz(u + 2u)) - 1;
}
static int nqi_int4bits(int d)
{
    unsigned u = log2(d);
    int s = nqi_bucket(u);
    return (s < 15 ? 4 : (s == 15 ? 5 : 6)) + s + 1 + 1;
}
typedef struct
{
    double bits;
} acc;

static double nqi_Hctx(unsigned* cnt, int nctx, int nsym) {
    double tot = 0; int c, k;
    for (c = 0; c < nctx; c++)
    {
        double N = 0;
        for (k = 0; k < nsym; k++)
            N += cnt[c * nsym + k];
        for (k = 0; k < nsym; k++)
        {
            double n = cnt[c * nsym + k];
            if (n > 0)
                tot += n * log2(N / n);
        }
    }
    return tot;
}

/* fenwick tree over use-timestamps for move-to-front rank */
static int* fw;
static size_t fwn;

static void fw_add(size_t i, int v)
{
    for (i++; i <= fwn; i += i & -i)
        fw[i] += v;
}
static int fw_sum(size_t i)
{
    int r = 0;
    for (i++; i > 0; i -= i & -i)
        r += fw[i];
    return r;
}

static double nqi_Hval(const int* d, size_t n, int sgn)
{
    static unsigned c[33 * 33];
    size_t i;
    int prev = 32;
    double x = 0;
    memset(c, 0, sizeof c);
    for (i = 0; i < n; i++)
    {
        int v = d[i], sym;
        if (v >= -8 && v <= 8)
            sym = v + 8;
        else
        {
            int s = nqi_bucket(v < 0 ? -v : v);
            sym = 17 + (s > 15 ? 15 : s);
            x += s + 1 + (sgn ? 1 : 0);
        }
        c[(prev > 32 ? 32 : prev) * 33 + sym]++;
        prev = sym;
    }
    return nqi_Hctx(c, 33, 33) + x;
}

static double nqi_H1(const unsigned* c, int n)
{
    double N = 0, t = 0;
    int i;
    for (i = 0; i < n; i++)
        N += c[i];
    
    for (i = 0; i < n; i++)
        if (c[i]) t += c[i] * log2(N / c[i]);
    
    return t;
}
static void nqi_raw_whatif(FILE* f)
{
    static unsigned cA[3][1024], cB[3][1024];
    size_t i;
    unsigned long nRaw = 0, nB = 0;
    int prev = 0;
#define NQI_CH(p, q, c) ( (c)==0 ? ((((p)>>8)&255)-(((q)>>8)&255)) : (c)==1 ? (((((p)>>16)&255)-(((q)>>16)&255)) - ((((p)>>8)&255)-(((q)>>8)&255))) : ((((p)&255)-((q)&255)) - ((((p)>>8)&255)-(((q)>>8)&255))) )
    for (i = 0; i < g_logn; i++)
    {
        const nqi_tok* t = &g_log[i];
        int c;
        if (t->k != 'W') continue;
        nRaw++;
        for (c = 0; c < 3; c++)
        cA[c][NQI_CH(t->v, prev, c) + 512]++;
        if (t->v3 >= 0)
        {
            nB++;
            for (c = 0; c < 3; c++)
                cB[c][NQI_CH(t->v, t->v3, c) + 512]++;
        }
        prev = t->v;
    }
    {
        double hA = 0, hB = 0; int c;
        for (c = 0; c < 3; c++)
        {
            hA += nqi_H1(cA[c], 1024);
            hB += nqi_H1(cB[c], 1024);
        }
        fprintf(f, "  raw pixels: %lu  = %lu B now (24 bits each)\n", nRaw, nRaw * 3);
        fprintf(f, "    delta vs previous palette entry, G-decorrelated, order-0 entropy : %9.0f B\n", hA / 8);
        fprintf(f, "    delta vs co-located ref / left pixel (%lu of them), rest raw      : %9.0f B\n", nB, hB / 8 + (nRaw - nB) * 3.0);
    }
}

static void nqi_analyze(FILE* f, unsigned long total_cmd_bits)
{
    nqi_raw_whatif(f);
    size_t nD_ = 0, i0, nlog = g_logn, tpos = 0;
    int* dEnd = malloc(sizeof(int) * nlog / 2 + 16), * dAbs = malloc(sizeof(int) * nlog / 2 + 16), * dMtf = malloc(sizeof(int) * nlog / 2 + 16);
    int* lastuse; long lastidx_ = -1, npel_ = 0;
    double mtfbits = 0;
    size_t nuse = 0;
    double bD_now = 0;
    double mean_rank = 0;
    for (i0 = 0; i0 < nlog; i0++)
        if (g_log[i0].k == 'D' || g_log[i0].k == 'W') nuse++;
    fwn = nuse + 2;
    fw = calloc(fwn + 2, sizeof(int)); lastuse = malloc(sizeof(int) * (nuse + 2));
    memset(lastuse, 0xff, sizeof(int) * (nuse + 2));
    for (i0 = 0; i0 < nlog; i0++)
    {
        nqi_tok* t = &g_log[i0];
        if (t->k == 'W')
        {
            lastuse[t->v2] = (int)tpos;
            fw_add(tpos++, 1);
            lastidx_ = t->v2;
            npel_++;
        }
        else if (t->k == 'D')
        {
            int idx = t->v2, rank = (int)(fw_sum(tpos) - fw_sum(lastuse[idx]));   /* entries used more recently */
            dEnd[nD_] = t->v; dAbs[nD_] = (int)(idx - (lastidx_ + 1));
            dMtf[nD_] = rank;
            mtfbits += nqi_int4bits(rank) - 1;
            nD_++;
            fw_add(lastuse[idx], -1);
            lastuse[idx] = (int)tpos;
            fw_add(tpos++, 1);
            lastidx_ = idx;
        }
    }
    {
        size_t k;
        for (k = 0; k < nD_; k++)
            bD_now += nqi_int4bits(dEnd[k]);
    }
    {
        size_t k;
        for (k = 0; k < nD_; k++) mean_rank += dMtf[k]; mean_rank /= (nD_ ? nD_ : 1);
    }
    fprintf(f, "  ref coding alternatives (bytes; value-level entropy + escape bits | plain int4):\n");
    fprintf(f, "    end-relative d (current)      : %9.0f\n", nqi_Hval(dEnd, nD_, 1) / 8);
    fprintf(f, "    abs pointer, expect last+1    : %9.0f\n", nqi_Hval(dAbs, nD_, 1) / 8);
    fprintf(f, "    recency rank (MTF)  [no sign] : %9.0f | %9.0f   (mean rank %.0f)\n", nqi_Hval(dMtf, nD_, 0) / 8, mtfbits / 8, mean_rank);
    {
        static const unsigned Ns[] = { 16, 64, 256, 1024, 4096, 16384, 65536 };
        size_t k, j;
        fprintf(f, "  recency-rank CDF (share of refs with rank < N) and hybrid cost (cache of N entries; miss = current coding):\n");
        for (j = 0; j < sizeof(Ns) / sizeof(Ns[0]); j++)
        {
            unsigned N = Ns[j];
            size_t hit = 0;
            double bits = 0;
            int smax = nqi_bucket(N - 1), sb = 0;
            while ((1 << sb) < smax + 1)
                sb++;
            for (k = 0; k < nD_; k++)
            {
                if ((unsigned)dMtf[k] < N)
                {
                    hit++;
                    bits += 1 + sb + nqi_bucket(dMtf[k]) + 1;
                }
                else
                    bits += 1 + nqi_int4bits(dEnd[k]);
            }
            fprintf(f, "    N=%6u : hit %5.1f%%   refdiff %9.0f B  (now %.0f B)\n", N, 100.0 * hit / (nD_ ? nD_ : 1), bits / 8, bD_now / 8);
        }
    }
    free(dEnd);
    free(dAbs);
    free(dMtf);
    free(fw);
    free(lastuse);

    {
        static unsigned cR[6 * 16], cK[10 * 4], cT[9], cD[19 * 18], cDa[19 * 18], cDe[19 * 18];
        size_t i;
        int bt = 0;
        unsigned long nR = 0, nE = 0, nT = 0, nD = 0, nW = 0, bR = 0, bF = 0, bT = 0, bD = 0;
        int prevR = 0, prevK = 4, prevB = 18, prevBa = 18, prevBe = 18, bt_cls = 0;
        long lastidx = -1;
        long npel = 0;
        int curdiff = 0;
        double dbitsA = 0, dbitsE = 0;
        for (i = 0; i < g_logn; i++)
        {
            nqi_tok* t = &g_log[i];
            switch (t->k)
            {
                case 'B':
                    bt = t->v; bt_cls = bt > 0; prevR = 0; prevK = 4; break;
                case 'R':
                    nR++; bR += nqi_gam(t->v); cR[prevR * 16 + t->v - 1]++; prevR = t->v > 5 ? 5 : t->v; break;
                case 'E':
                    nE++; bF += 1; cK[(bt_cls * 5 + prevK) * 4 + 0]++; prevK = 0; break;
                case 'T':
                    nT++; bF += 2; bT += nqi_gam(t->v); cK[(bt_cls * 5 + prevK) * 4 + 1]++; prevK = 1; cT[31 - __builtin_clz(t->v)]++; break;
                case 'D':
                {
                    int d = t->v;
                    int b = nqi_bucket(d < 0 ? -d : d);
                    int ea = (int)(t->v2 - (lastidx + 1));
                    int ba = nqi_bucket(ea < 0 ? -ea : ea);
                    nD++; bF += bt_cls ? 3 : 1; bD += nqi_int4bits(d);
                    cK[(bt_cls * 5 + prevK) * 4 + 2]++; prevK = 2;
                    cD[prevB * 18 + b]++; prevB = b;
                    cDa[prevBa * 18 + ba]++; prevBa = ba; dbitsA += nqi_int4bits(ea);
                    lastidx = t->v2; curdiff = (int)(npel - t->v2);
                    break;
                }
                case 'W': nW++; bF += bt_cls ? 3 : 1; cK[(bt_cls * 5 + prevK) * 4 + 3]++; prevK = 3; lastidx = t->v2; npel++; break;
            }
        }
        {
            double xD = 0, xDa = 0; { int c, k; for (c = 0; c < 19; c++) for (k = 0; k < 18; k++) { xD += (double)cD[c * 18 + k] * (k + 2); xDa += (double)cDa[c * 18 + k] * (k + 2); } }
            double xT = 0; { int k; for (k = 0; k < 9; k++) xT += (double)cT[k] * k; }
            double hR = nqi_Hctx(cR, 6, 16), hK = nqi_Hctx(cK, 10, 4), hT = nqi_Hctx(cT, 1, 9), hD = nqi_Hctx(cD, 19, 18), hDa = nqi_Hctx(cDa, 19, 18);
            double cur = bR + bF + bT + bD;
            fprintf(f, "  tokens: runs %lu  eq %lu  tbl %lu  ref %lu  raw %lu   blocks t0/t1/t2/t3 = %lu/%lu/%lu/%lu\n", nR, nE, nT, nD, nW, g_blk[0], g_blk[1], g_blk[2], g_blk[3]);
            fprintf(f, "  cmd bits now : runs %.0f  flags %lu  tblidx %lu  refdiff %lu  | token total %.0f  of cmd %lu (rest = block hdr/force/row flags %.0f)\n", (double)bR, bF, bT, bD, cur, total_cmd_bits, (double)total_cmd_bits - cur);
            fprintf(f, "  bytes now    : runs %.0f  flags %.0f  tblidx %.0f  refdiff %.0f\n", bR / 8.0, bF / 8.0, bT / 8.0, bD / 8.0);
            fprintf(f, "  bits/ref now %.2f   (int4 s-field = 4 bits, sign = 1)   avg bits per run-token %.2f\n", (double)bD / (nD ? nD : 1), (double)bR / (nR ? nR : 1));
            fprintf(f, "  ideal entropy-coded: runs %.0f B  kinds %.0f B  tblidx %.0f B  refdiff(end-rel) %.0f B  refdiff(abs-ptr) %.0f B\n",
                hR / 8, hK / 8, (hT + xT) / 8, (hD + xD) / 8, (hDa + xDa) / 8);
            fprintf(f, "  int4-coded diff if ABS-POINTER predictor: %.0f B (now %.0f B)\n", dbitsA / 8, bD / 8.0);
        }
    }
}

#endif