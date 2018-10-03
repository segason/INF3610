///////////////////////////////////////////////////////////////////////////////
//
//	main.cpp
//
///////////////////////////////////////////////////////////////////////////////
#include <systemc.h>
#include "Sobel.h"
#include "Reader.h"
#include "DataRAM.h"
#include "Writer.h"

#define RAMSIZE 0x200000

// Global variables
bool m_bError = false;

///////////////////////////////////////////////////////////////////////////////
//
//	Main
//
///////////////////////////////////////////////////////////////////////////////
int sc_main(int arg_count, char **arg_value)
{
	// Variables
	int sim_units = 2; //SC_NS 
	
	// Components
	// TODO: Instanciation des modules 
	Sobel sobelModule("Sobel");
	Reader readerModule("Reader");
	DataRAM dataRAMModule("DataRAM", "image.mem", RAMSIZE);
	Writer writerModule("Writer");

	// Connexions
	//TODO: Connexions des ports des modules
	readerModule.dataPortRAM(dataRAMModule);
	writerModule.dataPortRAM(dataRAMModule);
	sobelModule.readPort(readerModule);
	sobelModule.writePort(writerModule);


	// Démarrage de l'application
	if (!m_bError)
	{
		cout << "Démarrage de la simulation." << endl;	
		sc_start( -1, sc_core::sc_time_unit(sim_units) );
		cout << endl << "Simulation s'est terminée à " << sc_time_stamp();
	}
	// Fin du programme
	return 0;
}
