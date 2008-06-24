#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;



int main()
{

  FrameworkManager::Instance();

//  UserAlgorithmTest userTest;
//  userTest.RunAllTests();
  
 // Benchmark b;
 // b.RunPlusTest();
    
//#if defined _DEBUG
  //NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!

  IAlgorithm *loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
  //  loader->initialize();   
  loader->setPropertyValue("OutputWorkspace","ws");   
  loader->setPropertyValue("Filename","/mnt/isishome/wmx35332/Test/Data/GEM38370.raw");   
  
  loader->execute();    
  if (loader->isExecuted()){ std::cout << "RAW file loaded!" << std::endl; }    
  
  IAlgorithm *convert = FrameworkManager::Instance().createAlgorithm("ConvertUnits");   
  convert->setPropertyValue("InputWorkspace","ws");   
  convert->setPropertyValue("OutputWorkspace","ws");   
  convert->setPropertyValue("Target","MomentumTransfer");   
  convert->execute();   
  if (convert->isExecuted()){ std::cout << "Units converted!" << std::endl; }
  
//  Workspace* output = FrameworkManager::Instance().getWorkspace("outer");  

//#endif

  FrameworkManager::Instance().clear();
  exit(0);
}
