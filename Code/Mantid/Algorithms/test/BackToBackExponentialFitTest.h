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

    outputSpace = "B2BOuter";
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

    alg.setPropertyValue("I", "297.0");
    alg.setPropertyValue("a", "2.0");
    alg.setPropertyValue("b", "0.03");
    alg.setPropertyValue("c", "79400.0");
    alg.setPropertyValue("s", "8.0");
    alg.setPropertyValue("bk", "8.0");
  }

  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 20.9,0.1);
    dummy = alg.getProperty("I");
    TS_ASSERT_DELTA( dummy, 295.22 ,0.1);
    dummy = alg.getProperty("a");
    TS_ASSERT_DELTA( dummy, 2.477 ,0.1);
    dummy = alg.getProperty("b");
    TS_ASSERT_DELTA( dummy, 0.03 ,0.1);
    dummy = alg.getProperty("c");
    TS_ASSERT_DELTA( dummy, 79400.02 ,0.1);
    dummy = alg.getProperty("s");
    TS_ASSERT_DELTA( dummy, 7.98 ,0.1);
    dummy = alg.getProperty("bk");
    TS_ASSERT_DELTA( dummy, 7.88 ,0.1);
  }


private:
  BackToBackExponentialPeakFit alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*BACKTOBACKEXPONENTIALFITTEST_H_*/
