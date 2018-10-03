///////////////////////////////////////////////////////////////////////////////
//
//	main.cpp
//
///////////////////////////////////////////////////////////////////////////////
#include <systemc.h>
#include "Sobel.h"
#include "Sobelv2.h"
#include "Reader.h"
#include "DataRAM.h"
#include "CacheMem.h"
#include "Writer.h"

#define RAMSIZE 0x200000

///////////////////////////////////////////////////////////////////////////////
//
//	Main
//
///////////////////////////////////////////////////////////////////////////////
int sc_main(int arg_count, char **arg_value)
{
	// Variables
	int sim_units = 2; //SC_NS

	// Clock
	const sc_core::sc_time_unit simTimeUnit = SC_NS;
	const int clk_freq = 4000;
	sc_clock clk("SysClock", clk_freq, simTimeUnit, 0.5);

	// Components
	//TODO : Déclaration des modules
	Reader readerModule("Reader");
	Writer writerModule("Writer");
	DataRAM dataRAMModule("DataRAM", "image.mem", RAMSIZE, false);

	// Signals
	sc_signal<unsigned int, SC_MANY_WRITERS> data;
	sc_signal<unsigned int, SC_MANY_WRITERS> address;
	sc_signal<unsigned int*> addressData;
	sc_signal<unsigned int> length;
	sc_signal<bool, SC_MANY_WRITERS> reqRead;
	sc_signal<bool, SC_MANY_WRITERS> reqWrite;
	sc_signal<bool, SC_MANY_WRITERS> reqCache;
	sc_signal<bool, SC_MANY_WRITERS> ackReaderWriter;
	sc_signal<bool, SC_MANY_WRITERS> ackCache;
	// ...

	// Connexions
	// TODO: Ajouter connexions 
	readerModule.clk(clk);
	readerModule.data(data);
	readerModule.address(address);
	readerModule.request(reqRead);
	readerModule.ack(ackReaderWriter);
	readerModule.dataPortRAM(dataRAMModule);

	writerModule.clk(clk);
	writerModule.data(data);
	writerModule.address(address);
	writerModule.request(reqWrite);
	writerModule.ack(ackReaderWriter);
	writerModule.dataPortRAM(dataRAMModule);


	const bool utiliseCacheMem = true;

	if (!utiliseCacheMem) {
		Sobel sobel("Sobel");

		/* à compléter (connexion ports)*/
		sobel.clk(clk);
		sobel.data(data);
		sobel.address(address);
		sobel.requestRead(reqRead);
		sobel.requestWrite(reqWrite);
		sobel.ack(ackReaderWriter);

		// Démarrage de l'application
		cout << "Démarrage de la simulation." << endl;
		sc_start(-1, sc_core::sc_time_unit(sim_units));
		cout << endl << "Simulation s'est terminée à " << sc_time_stamp();
	} else {
		Sobelv2 sobel("Sobel");
		CacheMem cacheMem("CacheMem");

		/* à compléter (connexion ports)*/
		sobel.clk(clk);
		sobel.dataRW(data);
		sobel.address(address);
		sobel.addressRes(addressData);
		sobel.requestRead(reqRead);
		sobel.requestWrite(reqWrite);
		sobel.requestCache(reqCache);
		sobel.ackCache(ackCache);
		sobel.ackReaderWriter(ackReaderWriter);
		sobel.length(length);

		cacheMem.clk(clk);
		cacheMem.dataReader(data);
		cacheMem.address(address);
		cacheMem.addressData(addressData);
		cacheMem.requestFromCPU(reqCache);
		cacheMem.requestToReader(reqRead);
		cacheMem.ackToCPU(ackCache);
		cacheMem.ackFromReader(ackReaderWriter);
		cacheMem.length(length);


		// Démarrage de l'application
		cout << "Démarrage de la simulation." << endl;
		sc_start(-1, sc_core::sc_time_unit(sim_units));
		cout << endl << "Simulation s'est terminée à " << sc_time_stamp();

	}

	return 0;
}
