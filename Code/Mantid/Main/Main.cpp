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

//  UserAlgorithmTest userTest;
 // userTest.RunAllTests();
  
 // Benchmark b;
 // b.RunPlusTest(10584,2000);
  //b.RunPlusTest(15584,2000);
  //b.RunPlusTest(2584,2000);

//#if defined _DEBUG
  //NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!
  //load a raw file

  IAlgorithm* loader;
  loader = fm.createAlgorithm("LoadEventPreNeXus");
  loader->setPropertyValue("EventFilename", "../../../Test/Data/sns_event_prenexus/CNCS_7850_neutron_event.dat");
  loader->setPropertyValue("OutputWorkspace", "outerA");
  loader->execute();

  IAlgorithm* rebin;
  rebin = fm.createAlgorithm("Rebin");
  rebin->setPropertyValue("InputWorkspace", "outerA");
  rebin->setPropertyValue("OutputWorkspace", "outer1");
  rebin->setPropertyValue("Params", "0, 1e3, 100e3");
  rebin->execute();

  MatrixWorkspace_const_sptr input = boost::dynamic_pointer_cast<const MatrixWorkspace>
          (AnalysisDataService::Instance().retrieve("outer1"));

  loader = fm.createAlgorithm("LoadEventPreNeXus");
  loader->setPropertyValue("EventFilename", "../../../Test/Data/sns_event_prenexus/CNCS_7850_neutron_event.dat");
  loader->setPropertyValue("OutputWorkspace", "outerB");
  loader->execute();

  rebin = fm.createAlgorithm("Rebin");
  rebin->setPropertyValue("InputWorkspace", "outerB");
  rebin->setPropertyValue("OutputWorkspace", "outer2");
  rebin->setPropertyValue("Params", "0, 1e2, 100e3");
  rebin->execute();

  MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>
          (AnalysisDataService::Instance().retrieve("outer2"));

  const int numberOfSpectra = input->getNumberHistograms();

  PARALLEL_FOR2(input, output)
  for (int j = 0; j < numberOfSpectra; ++j)
	{
    PARALLEL_START_INTERUPT_REGION
    std::cout << "Copying X "<< j << ".\n";
    for (int dumb=0; dumb<1000; dumb++)
    {
    output->dataX(j) = input->readX(j);
    for (int i=0; i<output->dataX(j).size(); i++)
      output->dataX(j)[i] = input->readX(j)[i]*1.2345;
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  std::cout << "DONE!\n";

  fm.clear();
  exit(0);

}

