#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "nqi_common.h"
#include "nqic.h"
#include "nqid.h"
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

int encode_png(const char* filename, const uint8_t* rgba_data, uint32_t width, uint32_t height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;

    spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);

    spng_set_png_file(ctx, fp);

    struct spng_ihdr ihdr = {
        .width = width,
        .height = height,
        .bit_depth = 8,
        .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA
    };
    spng_set_ihdr(ctx, &ihdr);

    size_t image_size = width * height * 4;
    int ret = spng_encode_image(ctx, rgba_data, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

    if (ret != 0)
        ret = -1;

    spng_ctx_free(ctx);
    fclose(fp);
    return ret;
}

int main(int argc, char* argv[])
{
    nqic_context nqic = NULL;
    nqid_context nqid = NULL;
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

    if (is_encode) {
        unsigned char* output_data = NULL;
        size_t output_size = 0;
        FILE* output;
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
            clock_t begin;
            clock_t end;
            if (image_size < 8 || memcmp(image_data, "\x89PNG\r\n\x1a\n", 8)) {
                fprintf(stderr, "Unsupported image format\n");
                free(image_data);
                return 1;
            }
            begin = clock();
            decode_png(image_data, image_size, (void**)&image_decoded, &image_decoded_size, &w, &h);
            end = clock();
            printf("png took %f\n", (double)(end - begin) / CLOCKS_PER_SEC);
        }
        nqic_init(&nqic);
        nqic_load_image(nqic, (const unsigned char*)image_decoded, image_decoded_size, w, h);
        nqic_encode(nqic);
        nqic_flush(nqic);
        nqic_get_output(nqic, &output_data, &output_size);

        output = fopen(output_file, "wb");
        if (!output) {
            fprintf(stderr, "Failed to open output file: %s\n", output_file);
            free(image_data);
            free(image_decoded);
            return 1;
        }
        fwrite(output_data, 1, output_size, output);
        fclose(output);
        nqic_free(&nqic);
    } else {
        unsigned char* output_data = NULL;
        size_t output_size = 0;
        unsigned short w, h;
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
        nqid_init(&nqid);
        nqid_load_image(nqid, image_data, image_size);
        nqid_get_info(nqid, &w, &h, NULL);
        nqid_decode(nqid);
        nqid_get_output(nqid, &output_data, &output_size);
        encode_png(output_file, output_data, w, h);
        nqic_free(&nqid);
    }
    return 0;
}