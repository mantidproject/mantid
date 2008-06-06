#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;



int main()
{

  FrameworkManager::Instance();

  UserAlgorithmTest userTest;
  userTest.RunAllTests();
	
 // Benchmark b;
 // b.RunPlusTest();
    
#if defined _DEBUG
	//NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!


#endif
  FrameworkManager::Instance().clear();
	exit(0);
}
