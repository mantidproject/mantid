// If you get the message  �This application has failed to start because MSVCR80.dll was not found. Re-installing the application may fix this problem.�
// when running to run this main.cpp in debug mode then try to uncomment the line below (see also http://blogs.msdn.com/dsvc/archive/2008/08/07/part-2-troubleshooting-vc-side-by-side-problems.aspx for more details)
//#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' \"") 

#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 
#include "MantidDataObjects/EventWorkspace.h"

#include <boost/timer.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

void test();

int main()
{
  FrameworkManagerImpl& fm = FrameworkManager::Instance();

  fm.clear();
  exit(0);

}

