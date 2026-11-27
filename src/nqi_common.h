#ifndef _NQI_COMMON_H_
#define _NQI_COMMON_H_

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