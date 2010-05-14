// If you get the message  “This application has failed to start because MSVCR80.dll was not found. Re-installing the application may fix this problem.”
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

#include <boost/timer.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

void test();

int main()
{

  FrameworkManagerImpl& fm = FrameworkManager::Instance();

//  UserAlgorithmTest userTest;
 // userTest.RunAllTests();
  
 // Benchmark b;
 // b.RunPlusTest(10584,2000);
  //b.RunPlusTest(15584,2000);
  //b.RunPlusTest(2584,2000);

//#if defined _DEBUG
  //NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!
  //load a raw file
    IAlgorithm* loader = fm.createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../Test/Data/MER02257.raw");

    std::string outputSpace = "outer";
    loader->setPropertyValue("OutputWorkspace", outputSpace);    
    loader->execute();

  Workspace* w = fm.getWorkspace(outputSpace);
  Workspace2D* output2D = dynamic_cast<Workspace2D*>(w);
  const int numberOfSpectra = output2D->getNumberHistograms();
      clock_t start = clock();
  int FailCount =0;
  int SuccessCount = 0;
  V3D total;
  for (int j = 0; j <= numberOfSpectra; ++j) 
	{
    try{
		// Now get the detector to which this relates
		IDetector_const_sptr det = output2D->getDetector(j);
    // Solid angle should be zero if detector is masked ('dead')
    V3D v = det->getPos();
    total += v;
    SuccessCount++;
  }
      catch (...)
      { 
        FailCount++;
      }
	} // loop over spectra
  clock_t end = clock();
    std::cout << double(end - start)/CLOCKS_PER_SEC << std::endl;
    std::cout << "Success " << SuccessCount << " | Failed " << FailCount << std::endl;
    std::cout << total << std::endl;
//#endif


  fm.clear();
  exit(0);

}

