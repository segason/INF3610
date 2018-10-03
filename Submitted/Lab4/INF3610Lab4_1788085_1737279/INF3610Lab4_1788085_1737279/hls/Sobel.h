#pragma once

#include <inttypes.h>


#define IMG_WIDTH 1920
#define IMG_HEIGHT 1080
#define IMG_SIZE (IMG_WIDTH * IMG_HEIGHT)
#define CH_HEIGHT 4
#define PIXEL_COUNT 4

void sobel_filter(uint8_t * inter_pix, unsigned int * out_pix);
uint8_t sobel_operator(const int col, const int row, uint8_t cache[CH_HEIGHT][IMG_WIDTH]);
