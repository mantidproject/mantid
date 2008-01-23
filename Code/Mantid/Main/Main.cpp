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
using namespace Mantid::Algorithms;



int main()
{

  FrameworkManager fm;
  //fm.initialize();

  Benchmark b;
  b.RunPlusTest();
    
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!


#endif
	exit(0);
}
