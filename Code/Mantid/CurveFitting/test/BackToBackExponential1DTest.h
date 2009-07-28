#ifndef BACKTOBACKEXPONENTIAL1DTEST_H_
#define BACKTOBACKEXPONENTIAL1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BackToBackExponential1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::BackToBackExponential1D;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class BackToBackExponential1DTest : public CxxTest::TestSuite
{
public:

  BackToBackExponential1DTest()
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
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79280"); 
    alg.setPropertyValue("EndX","79615");   

    alg.setPropertyValue("I", "297.0");
    alg.setPropertyValue("A", "2.0");
    alg.setPropertyValue("B", "0.03");
    alg.setPropertyValue("X0", "79400.0");
    alg.setPropertyValue("S", "8.0");
    alg.setPropertyValue("BK", "8.0");
  }

  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 9.15,0.1);
    dummy = alg.getProperty("I");
    TS_ASSERT_DELTA( dummy, 294.37 ,0.1);
    dummy = alg.getProperty("A");
    TS_ASSERT_DELTA( dummy, 2.38 ,0.1);
    dummy = alg.getProperty("B");
    TS_ASSERT_DELTA( dummy, 0.03 ,0.1);
    dummy = alg.getProperty("X0");
    TS_ASSERT_DELTA( dummy, 79400.02 ,0.1);
    dummy = alg.getProperty("S");
    TS_ASSERT_DELTA( dummy, 8.15 ,0.1);
    dummy = alg.getProperty("BK");
    TS_ASSERT_DELTA( dummy, 7.88 ,0.1);
  }


private:
  BackToBackExponential1D alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*BACKTOBACKEXPONENTIAL1DTEST_H_*/
