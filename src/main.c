#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "nqi_common.h"
#include "nqic.h"
#include "spng/spng.h"

unsigned char* image_data = NULL;
unsigned int image_size = 0;
unsigned int* image_decoded = NULL;
unsigned int image_decoded_size = 0;

int decode_png(const void *png_buf, size_t png_size, void **out_pixels, size_t *out_size, int *width, int *height) {
    spng_ctx *ctx = spng_ctx_new(0);
    struct spng_ihdr ihdr;
    if (!ctx) return -1;

    int ret = spng_set_png_buffer(ctx, png_buf, png_size);
    if (ret) goto cleanup;

    ret = spng_get_ihdr(ctx, &ihdr);
    if (ret) goto cleanup;

    ret = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, out_size);
    if (ret) goto cleanup;

    *out_pixels = malloc(*out_size);
    if (!*out_pixels) {
        ret = -1;
        goto cleanup;
    }

    ret = spng_decode_image(ctx, *out_pixels, *out_size, SPNG_FMT_RGBA8, 0);
    if (ret) {
        free(*out_pixels);
        *out_pixels = NULL;
    }

    *width = ihdr.width;
    *height = ihdr.height;

cleanup:
    spng_ctx_free(ctx);
    return ret;
}

int main(int argc, char* argv[])
{
    nqic_context nqic = NULL;
    char* image_file = NULL;
    char* output_file = NULL;
    int w = 0, h = 0;
    bool is_encode = false;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <c/d> <image_file> <output_file>\n", argv[0]);
        return 1;
    }

    if (argv[1][0] != 'c' && argv[1][0] != 'd') {
        fprintf(stderr, "Invalid operation: %s. Use 'c' for compress or 'd' for decompress.\n", argv[1]);
        return 1;
    }

    if (argv[1][0] == 'c') {
        is_encode = true;
    } else {
        is_encode = false;
    }

    image_file = argv[2];
    output_file = argv[3];
    
    // read image data
    {
        FILE* input = fopen(image_file, "rb");
        if (!input) {
            fprintf(stderr, "Failed to open input file: %s\n", image_file);
            return 1;
        }
        fseek(input, 0, SEEK_END);
        image_size = ftell(input);
        fseek(input, 0, SEEK_SET);
        image_data = malloc(image_size);
        if (!image_data) {
            fprintf(stderr, "Failed to allocate memory for image data\n");
            fclose(input);
            return 1;
        }
        fread(image_data, 1, image_size, input);
        fclose(input);
    }

    // parse
    {
        //Currently, only PNG is supported
        if (image_size < 8 || memcmp(image_data, "\x89PNG\r\n\x1a\n", 8)){
            fprintf(stderr, "Unsupported image format\n");
            free(image_data);
            return 1;
        }
        decode_png(image_data, image_size, (void**)&image_decoded, &image_decoded_size, &w, &h);
    }

    if (is_encode) {
        unsigned char* output_data = NULL;
        size_t output_size = 0;
        nqic_init(&nqic);
        nqic_load_image(nqic, (const unsigned char*)image_decoded, image_decoded_size, w, h);
        nqic_encode(nqic);
        nqic_flush(nqic);
        nqic_get_output(nqic, &output_data, &output_size);

        FILE* output = fopen(output_file, "wb");
        if (!output) {
            fprintf(stderr, "Failed to open output file: %s\n", output_file);
            free(image_data);
            free(image_decoded);
            free(output_data);
            return 1;
        }
        fwrite(output_data, 1, output_size, output);
        fclose(output);
        free(output_data);
    } else {
        printf("Decompression is not implemented yet.\n");
        return 1;
    }
    return 0;
}