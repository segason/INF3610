#include "Sobel.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#define ABS(x)          ((x>0)? x : -x)

typedef union {
	uint8_t pix[4];
	unsigned full;
} OneToFourPixels;

uint8_t sobel_operator(const int col, const int row, uint8_t cache[CH_HEIGHT][IMG_WIDTH])
{
#pragma HLS inline			// Inliner la fonction lui permet d'être "copiée-collée" là où elle est appellée
							// et ainsi faciliter le pipelinage de la boucle principale
	/* À compléter en important votre code du lab 3.
	 * À noter que la fonction peut avoir 3 signatures différentes, selon vos différentes modifications:
	 * uint8_t sobel_operator(const int fullIndex, uint8_t * image)
	 * uint8_t sobel_operator(const int fullIndex, uint8_t image[IMG_HEIGHT * IMG_WIDTH])
	 * uint8_t sobel_operator(const int col, const int row, uint8_t image[IMG_HEIGHT][IMG_WIDTH])
	 *
	 * Les deux premières sont assez équivalentes, mais la dernière permet d'accéder à l'image comme un
	 * tableau 2D. Par contre, un tableau 2D doit alors lui être passé, ce qui n'est pas évident considérant
	 * que les entrées de la fonction sobel_filtrer() sont 1D. Cependant, si pour une raison ou une autre
	 * un buffer-cache intermédiaire était utilisé, celui-ci pourrait être 2D...
	 */

	int x_weight = 0;
	int y_weight = 0;

	unsigned edge_weight;
	uint8_t edge_val;

	const char x_op[3][3] = { { -1,0,1 },
								{ -2,0,2 },
								{ -1,0,1 } };

	const char y_op[3][3] = { { 1,2,1 },
								{ 0,0,0 },
								{ -1,-2,-1 } };

	//Compute approximation of the gradients in the X-Y direction
	int cacheColIndex = 0, cacheRowIndex = 0;
	X_direction:
	for (int i = 0; i < 3; i++) {
		Y_direction:
		for (int j = 0; j < 3; j++) {
			cacheColIndex = (col + i - 1 + CH_HEIGHT) % CH_HEIGHT;
			cacheRowIndex = row + j - 1;
			// X direction gradient
			x_weight = x_weight + cache[cacheColIndex][cacheRowIndex] * x_op[i][j];

			// Y direction gradient
			y_weight = y_weight + cache[cacheColIndex][cacheRowIndex] * y_op[i][j];
		}
	}

	edge_weight = ABS(x_weight) + ABS(y_weight);

	edge_val = (255 - (uint8_t)(edge_weight));

	//Edge thresholding
	if (edge_val > 200)
		edge_val = 255;
	else if (edge_val < 100)
		edge_val = 0;

	return edge_val;
}


void sobel_filter(uint8_t * inter_pix, unsigned int * out_pix)
{

	#pragma HLS INTERFACE m_axi port=inter_pix offset=slave
	#pragma HLS INTERFACE m_axi port=out_pix offset=slave
	#pragma HLS INTERFACE s_axilite port=return

	uint8_t lineBuffer[CH_HEIGHT][IMG_WIDTH];
	#pragma HLS ARRAY_PARTITION variable=lineBuffer complete dim=1
	LineLoop:
	for (unsigned int i = 0; i < (CH_HEIGHT / 2); i++) {
		ColumnLoop:
		for (unsigned int j = 0; j < IMG_WIDTH; j++) {
			lineBuffer[i][j] = inter_pix[i*IMG_WIDTH + j];
		}
	}

	int fullIndex = 0, newCacheIndex = 0, newInterPixIndex = 0;
	OneToFourPixels tempPixel;
	ExternalLoop:
	for(unsigned int i = 0; i < IMG_HEIGHT; i++){
		InternalLoop:
		for(unsigned int j = 0; j < IMG_WIDTH; j++){
			#pragma HLS pipeline
			#pragma HLS loop_flatten off
			if(i == 0 || i == IMG_HEIGHT - 1|| j == IMG_WIDTH - 1 || j == 0)
				tempPixel.pix[0] = 0;
			else
				tempPixel.pix[0] = sobel_operator(i % CH_HEIGHT, j, lineBuffer);

			newCacheIndex = (i + (CH_HEIGHT / 2)) % CH_HEIGHT;
			newInterPixIndex = ((i + (CH_HEIGHT / 2)) % IMG_HEIGHT) * IMG_WIDTH + j;
			lineBuffer[newCacheIndex][j] = inter_pix[newInterPixIndex];

			fullIndex = i*IMG_WIDTH + j;
			OneTo4:
			for(unsigned int i = 1; i < PIXEL_COUNT; i++){
				tempPixel.pix[i] = tempPixel.pix[0];
			}
			out_pix[fullIndex] = tempPixel.full;
		}
	}

}
