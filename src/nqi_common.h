#ifndef _NQI_COMMON_H_
#define _NQI_COMMON_H_

// ========================= INLINE ===========================

#ifdef _MSC_VER
#define NQI_INLINE static __forceinline
#else
#define NQI_INLINE static inline __attribute__((always_inline))
#endif

// ========================= BITSWAP ==========================

#define      NQI_BSWAP(tmp) \
{\
  register unsigned int  t; \
  t = tmp ^ ((tmp << 16) | (tmp >> 16)); \
  t &= ~0xff0000; \
  tmp = (tmp << 24) | (tmp >> 8); \
  tmp = tmp ^ (t >> 8); \
}

// ======================== METADATA ==========================

typedef enum {
    NQIM_META_INVALID = 0,
    NQIM_META_WHOLE_IMAGE = 1,
    NQIM_META_IMAGE_DATA = 2,
    //... rest to follow soon
    NQIM_META_END = 0xff,
} nqim_meta_type;

typedef struct {
    unsigned int size;
    nqim_meta_type type;
    unsigned char data[1];
} nqim_metadata;

#endif /* _NQI_COMMON_H_ */