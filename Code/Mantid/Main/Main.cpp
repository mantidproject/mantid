#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;



int main()
{

  FrameworkManager* fm = FrameworkManager::Instance();
  fm->initialize();

  Benchmark b;
  b.RunPlusTest();
    
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!


#endif
  fm->clear();
	exit(0);
}
