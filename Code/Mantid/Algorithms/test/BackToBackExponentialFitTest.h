#ifndef BACKTOBACKEXPONENTIALFITTEST_H_
#define BACKTOBACKEXPONENTIALFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BackToBackExponentialPeakFit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class BackToBackExponentialPeakFitTest : public CxxTest::TestSuite
{
public:
  
  BackToBackExponentialPeakFitTest()
  {   
    std::string inputFile = "../../../../Test/Data/HRP38692.RAW";

    LoadRaw loader;

    loader.initialize();

    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);    
    
    loader.execute();
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());    
    TS_ASSERT( alg.isInitialized() );    
    
    // Set the properties
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("SpectrumNumber","3");
    alg.setPropertyValue("StartX","20712"); // correspond to about 79250 ms
    alg.setPropertyValue("EndX","20755");   // correspond to about 79615 ms

    alg.setPropertyValue("Output I", "297.0");
    alg.setPropertyValue("Output a", "2.0");
    alg.setPropertyValue("Output b", "0.03");
    alg.setPropertyValue("Output c", "79400.0");
    alg.setPropertyValue("Output s", "8.0");
    alg.setPropertyValue("Output bk", "8.0");
  }
  
  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 11.11,0.1);
    dummy = alg.getProperty("Output I");
    TS_ASSERT_DELTA( dummy, 294.67 ,0.1);
    dummy = alg.getProperty("Output a");
    TS_ASSERT_DELTA( dummy, 2.477 ,0.1);
    dummy = alg.getProperty("Output b");
    TS_ASSERT_DELTA( dummy, 0.03 ,0.1);
    dummy = alg.getProperty("Output c");
    TS_ASSERT_DELTA( dummy, 79400.02 ,0.1);
    dummy = alg.getProperty("Output s");
    TS_ASSERT_DELTA( dummy, 7.98 ,0.1);
    dummy = alg.getProperty("Output bk");
    TS_ASSERT_DELTA( dummy, 7.88 ,0.1);
  }
  
  
private:
  BackToBackExponentialPeakFit alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*BACKTOBACKEXPONENTIALFITTEST_H_*/
