///////////////////////////////////////////////////////////////////////////////
//
//	Sobelv2.cpp
//
///////////////////////////////////////////////////////////////////////////////
#include "Sobelv2.h"


///////////////////////////////////////////////////////////////////////////////
//
//	Constructeur
//
///////////////////////////////////////////////////////////////////////////////
Sobelv2::Sobelv2( sc_module_name name ) : sc_module(name)
/* À compléter */
{
	/*
	
	À compléter
	
	*/
	SC_THREAD(thread);
	sensitive << clk.pos();
}


///////////////////////////////////////////////////////////////////////////////
//
//	Destructeur
//
///////////////////////////////////////////////////////////////////////////////
Sobelv2::~Sobelv2()
{
	/*
	
	À compléter
	
	*/
}





void Sobelv2::Write(const unsigned int addressWR, const unsigned int dataWR)
{
	address.write(addressWR);
	dataRW.write(dataWR);
	requestWrite.write(true);
	do {
		wait(clk->posedge_event());
	} while (!ackReaderWriter.read());
	requestWrite.write(false);
}

void Sobelv2::ReadCache(const unsigned int addressRDC, unsigned int * dataAddress, const unsigned int lengthImg)
{
	address.write(addressRDC);
	addressRes.write(dataAddress);
	length.write(lengthImg);
	requestCache.write(true);
	do {
		wait(clk->posedge_event());
	} while (!ackReaderWriter.read());
	requestCache.write(false);
}

unsigned int Sobelv2::Read(const unsigned int addressRD)
{
	int receivedData = 0;
	address.write(addressRD);
	requestRead.write(true);
	do {
		wait(clk->posedge_event());
	} while (!ackReaderWriter.read());
	receivedData = dataRW.read();
	requestRead.write(false);
	return receivedData;
}


///////////////////////////////////////////////////////////////////////////////
//
//	thread
//
///////////////////////////////////////////////////////////////////////////////
void Sobelv2::thread(void)
{
	/*

	À compléter

	*/

	unsigned int address = 0;
	unsigned int imgWidth, imgHeight;

	while (true) {
		imgWidth = Read(address);
		address += 4;
		imgHeight = Read(address);
		unsigned int imgSize = imgWidth*imgHeight;

		//Create array
		uint8_t * image = new uint8_t[imgSize];
		uint8_t * cache = new uint8_t[4 * imgWidth];
		uint8_t * result = new uint8_t[imgSize];
		int * imageAsInt = reinterpret_cast<int*>(image);
		int * resultAsInt = reinterpret_cast<int*>(result);

		address += 4;
		ReadCache(address, (unsigned int*)cache, 3 * imgWidth);
		address += 3 * imgWidth;

		unsigned int * cacheData;
		//Calling the operator for each pixel
		for (unsigned int i = 1; i < imgHeight - 1; i++) {
			cacheData = (unsigned int *)cache + ((address - 8) % (4 * imgWidth) / 4);
			ReadCache(address, cacheData, imgWidth);
			address += imgWidth;
			for (unsigned int j = 1; j < imgWidth - 1; j++) {
				wait(12);
				int fullIndex = i * imgWidth + j;
				int sobelIndex = (address - 8 - 3 * imgWidth) % (imgWidth * 4) + j;
				result[fullIndex] = Sobelv2_operator(sobelIndex, imgWidth, cache);
			}
		}


		//Write back nb. elements at the end
		address = 4;
		for (unsigned int i = 0; i < imgSize / sizeof(int); i++) {
			//Write each element
			address += 4;
			if (i == 0 || i == imgHeight - 1 || i == imgWidth - 1)
				Write(address, 0);
			else
				Write(address, resultAsInt[i]);
		}

		delete(cache);
		delete(result);
		sc_stop();
	}

}



///////////////////////////////////////////////////////////////////////////////
//
//	Sobelv2_operator
//
///////////////////////////////////////////////////////////////////////////////
static inline uint8_t getVal(int index, int xDiff, int yDiff, int img_width, uint8_t * Y)
{
	int fullIndex = (index + (yDiff * img_width)) + xDiff;
	if (fullIndex < 0)
	{
		//Cas ou on doit chercher la derniere ligne
		fullIndex += img_width * 4;
	}
	else if (fullIndex >= img_width * 4)
	{
		//Cas ou on doit aller chercher la premiere ligne
		fullIndex -= img_width * 4;
	}

	return Y[fullIndex];
};

uint8_t Sobelv2::Sobelv2_operator(const int index, const int imgWidth, uint8_t * image)
{
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
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			// X direction gradient
			x_weight = x_weight + (getVal(index, i - 1, j - 1, imgWidth, image) * x_op[i][j]);

			// Y direction gradient
			y_weight = y_weight + (getVal(index, i - 1, j - 1, imgWidth, image) * y_op[i][j]);
		}
	}

	edge_weight = std::abs(x_weight) + std::abs(y_weight);

	edge_val = (255 - (uint8_t)(edge_weight));

	//Edge thresholding
	if (edge_val > 200)
		edge_val = 255;
	else if (edge_val < 100)
		edge_val = 0;

	return edge_val;
}