#ifndef LOADEMPTYINSTRUMENTTEST_H_
#define LOADEMPTYINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Component.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadEmptyInstrumentTest : public CxxTest::TestSuite
{
public:

  LoadEmptyInstrumentTest()
  {
  }


  void testExecSLS()
  {
    LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT( loaderSLS.isInitialized() );
    inputFile = "../../../../Test/Instrument/SLS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);
    wsName = "LoadEmptyInstrumentTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT( loaderSLS.isExecuted() );


    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    
    // Check the total number of elements in the map for SLS
    TS_ASSERT_EQUALS(output->spectraMap().nElements(),683);
  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADEMPTYINSTRUMENTTEST_H_*/
