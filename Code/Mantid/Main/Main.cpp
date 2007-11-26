#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/Instrument.h"
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "Component.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
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
		loader->setProperty("OutputWorkspace","WSInstrument");
		loader->execute();

		Workspace *ws = manager.getWorkspace("WSInstrument");
		Instrument& i = ws->getInstrument();
		i.printSelf(std::cout);
		std::cout << std::endl;
		Mantid::Geometry::Component* source = i[0];
		Mantid::Geometry::Component* samplepos = i[1];
		i.printChildren(std::cout);
		std::cout <<std::endl;

		i.getDetector(1000)->printSelf(std::cout);
		std::cout <<i.getDetector(1000)->getID()<<std::endl;

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