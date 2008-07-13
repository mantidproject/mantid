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
  }
  
  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );
  }
  
  
private:
  GaussLeastSquaresFit alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*GAUSSLEASTSQUARESFITTEST_H_*/
