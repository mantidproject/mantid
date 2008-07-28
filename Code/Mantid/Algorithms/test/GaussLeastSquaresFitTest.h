#ifndef GAUSSLEASTSQUARESFITTEST_H_
#define GAUSSLEASTSQUARESFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GaussLeastSquaresFit.h"
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

class GaussLeastSquaresFitTest : public CxxTest::TestSuite
{
public:

  GaussLeastSquaresFitTest()
  {
    std::string inputFile = "../../../../Test/Data/MAR11060.RAW";

    LoadRaw loader;

    loader.initialize();

    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "GaussOuter";
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

    alg.setPropertyValue("Output y0", "-2000.0");
    alg.setPropertyValue("Output A", "8000.0");
    alg.setPropertyValue("Output xc", "10000.0");
    alg.setPropertyValue("Output w", "6000.0");

  }

  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 101.027,0.01);
    dummy = alg.getProperty("Output y0");
    TS_ASSERT_DELTA( dummy, -2509.1 ,0.1);
    dummy = alg.getProperty("Output A");
    TS_ASSERT_DELTA( dummy, 8618.4 ,0.1);
    dummy = alg.getProperty("Output xc");
    TS_ASSERT_DELTA( dummy, 10080.6 ,0.1);
    dummy = alg.getProperty("Output w");
    TS_ASSERT_DELTA( dummy, 6349.74 ,0.1);
  }


private:
  GaussLeastSquaresFit alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*GAUSSLEASTSQUARESFITTEST_H_*/
