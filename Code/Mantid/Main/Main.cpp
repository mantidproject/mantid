#include <iostream>
#include <iomanip>
//#include "Benchmark.h"
//#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;



int main()
{

  FrameworkManager::Instance();

  
  IAlgorithm *loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
//  loader->initialize();
  loader->setPropertyValue("OutputWorkspace","ws");
  loader->setPropertyValue("Filename","/mnt/isishome/wmx35332/Test/Data/GEM38203.raw");
  
  loader->execute();
  if (loader->isExecuted()){ std::cout << "RAW file loaded!" << std::endl; }
  
  IAlgorithm *convert = FrameworkManager::Instance().createAlgorithm("ConvertUnits");
  convert->setPropertyValue("InputWorkspace","ws");
  convert->setPropertyValue("OutputWorkspace","outer");
  convert->setPropertyValue("Target","MomentumTransfer");
  convert->execute();
  if (convert->isExecuted()){ std::cout << "Units converted!" << std::endl; }
  
//  UserAlgorithmTest userTest;
//  userTest.RunAllTests();
	
 // Benchmark b;
 // b.RunPlusTest();
    
#if defined _DEBUG
	//NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!


#endif
//  FrameworkManager::Instance().clear();
	exit(0);
}
