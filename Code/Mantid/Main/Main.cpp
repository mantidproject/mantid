#include <iostream>
#include <iomanip>

#include "LoadInstrument.h"
#include "SimpleIntegration.h"
#include "Exception.h"
#include "FrameworkManager.h"
#include "WorkspaceFactory.h"
#include "Algorithm.h"


using namespace Mantid::Kernel;
int
main()
{
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!
	FrameworkManager manager;
	manager.initialize();
	try
	{
		IAlgorithm *loader = manager.createAlgorithm("LoadInstrument");
		std::string inputFile("../../../Test/Instrument/HET_Definition.txt");
		loader->setProperty("Filename",inputFile);
		loader->execute();
		delete loader;
	}
	catch (Exception::NotFoundError &ex)
	{
	  std::cerr << ex.what() <<std::endl;
	}
	   
	//char c[10];
	//std::cin.getline( &c[0], 1, '2' );

	manager.clear();
#endif
	exit(0);
}