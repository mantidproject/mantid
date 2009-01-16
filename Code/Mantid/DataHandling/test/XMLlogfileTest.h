#ifndef XMLLOGFILETEST_H_
#define XMLLOGFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/XMLlogfile.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class XMLlogfileTest : public CxxTest::TestSuite
{
public:

  // LoadRaw2 uses XMLlogfile to populate its parameter map. Hence the test here simply
  // checks that this is done ok
  void testParameterMap()
  {
    LoadRaw2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/Data/Full point detector CRISP dataset/csp79590.raw");
    loader.setPropertyValue("OutputWorkspace", "CRISPdata");

    TS_ASSERT_THROWS_NOTHING( loader.execute() )
    TS_ASSERT( loader.isExecuted() )

    // Get back the workspaces
    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING( output1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("CRISPdata")) );
    TS_ASSERT_EQUALS( output1->getNumberHistograms(), 4 )
    MatrixWorkspace_sptr output2;
    TS_ASSERT_THROWS_NOTHING( output2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("CRISPdata_2")) );
    TS_ASSERT_EQUALS( output2->getNumberHistograms(), 4 )

    // get the parameter map for the period 1 CRISP data
    boost::shared_ptr<ParameterMap> paramMap = output1->InstrumentParameters();

    // check that parameter have been read into the instrument parameter map
    std::vector<double> ret1 = paramMap->getDouble("point-detector", "z");
    TS_ASSERT_EQUALS( ret1.size(), 1 );
    std::vector<double> ret2 = paramMap->getDouble("linear-detector", "z");
    TS_ASSERT_EQUALS( ret2.size(), 1 );

  }
};

#endif /*XMLLOGFILETEST_H_*/
