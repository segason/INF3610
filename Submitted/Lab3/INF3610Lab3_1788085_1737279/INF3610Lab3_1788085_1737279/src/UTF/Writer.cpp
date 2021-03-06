///////////////////////////////////////////////////////////////////////////////
//
//	Writer.cpp
//
///////////////////////////////////////////////////////////////////////////////
#include "Writer.h"

///////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
///////////////////////////////////////////////////////////////////////////////
Writer::Writer(sc_module_name zName)
	: sc_channel(zName)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
///////////////////////////////////////////////////////////////////////////////
Writer::~Writer()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//	read
//
///////////////////////////////////////////////////////////////////////////////
void Writer::Write(unsigned int addr, unsigned int data)
{
	dataPortRAM->Write(addr, data);
}
