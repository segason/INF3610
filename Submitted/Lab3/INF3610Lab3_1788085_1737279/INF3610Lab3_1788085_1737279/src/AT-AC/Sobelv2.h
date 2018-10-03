///////////////////////////////////////////////////////////////////////////////
//
//	Sobelv2.h
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <systemc.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
//
//	Class Sobelv2
//
///////////////////////////////////////////////////////////////////////////////
class Sobelv2 : public sc_module
{
	public: 
		// Ports
		//Communication avec reader et writer
		sc_in_clk				clk;  
		sc_inout<unsigned int>  dataRW;
		sc_out<unsigned int>  	address;
		sc_out<bool>			requestRead;
		sc_out<bool>			requestWrite;
		sc_in<bool>				ackReaderWriter;

		//Communication avec cache
		sc_out<unsigned int*>	addressRes;
		sc_out<unsigned int>  	length;
		sc_out<bool>			requestCache;
		sc_in<bool>				ackCache;
	
		// Constructor
		Sobelv2( sc_module_name name );
		
		// Destructor
		~Sobelv2();
		
	private:
		// Process SystemC
		/*
			À compléter
		*/
		SC_HAS_PROCESS(Sobelv2);
		
		void thread(void);
		void Sobelv2::Write(const unsigned int address, const unsigned int dataRW);
		void Sobelv2::ReadCache(const unsigned int adress, unsigned int * dataAddress, const unsigned int length);
		unsigned int Sobelv2::Read(const unsigned int address);
		uint8_t Sobelv2_operator(const int index, const int imgWidth, uint8_t * image);
};
