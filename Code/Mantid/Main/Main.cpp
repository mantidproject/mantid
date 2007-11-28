#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataHandling/LoadLog.h"
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
		IAlgorithm *loader = manager.createAlgorithm("LoadLog");
		std::string inputFile("../../../Test/Data/HRP37129_ICPevent.txt");
		loader->setProperty("Filename",inputFile);
		//loader->setProperty("OutputWorkspace","WSInstrument");
		loader->execute();

    /*
    IAlgorithm *saveCSV = manager.createAlgorithm("SaveCSV");
    saveCSV->setProperty("Filename", "saveCSV2.txt");
    saveCSV->setProperty("InputWorkspace", "inFile");
    saveCSV->setProperty("Seperator", "*");
    saveCSV->setProperty("LineSeperator", "\n");
    saveCSV->initialize();
    saveCSV->execute();
    saveCSV->finalize();		
*/
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