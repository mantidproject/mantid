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
    loader->setPropertyValue("Filename", "../../../Test/Data/MER02257.raw");

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

