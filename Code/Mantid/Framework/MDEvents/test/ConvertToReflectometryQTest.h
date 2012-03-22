#ifndef MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_
#define MANTID_MDEVENTS_CONVERTTOREFLECTOMETRYQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"
#include "MantidAPI/IMDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class ConvertToReflectometryQTest : public CxxTest::TestSuite
{
private:

  void doExecute(const std::string& outWSName)
  {
    MatrixWorkspace_sptr in_ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    in_ws->getAxis(0)->setUnit("TOF");
  
    ConvertToReflectometryQ alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", in_ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToReflectometryQTest *createSuite() { return new ConvertToReflectometryQTest(); }
  static void destroySuite( ConvertToReflectometryQTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToReflectometryQ alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_check_output_metadata()
  {
    std::string outWSName("ConvertToReflectometryQTest_OutputWS");

    doExecute(outWSName);

    IMDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName) );
    TS_ASSERT(ws);

    //Quick metadata checks associated with the ouput workspace
    TSM_ASSERT_EQUALS("Wrong number of bins", 100, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 2, ws->getNumDims());
    TSM_ASSERT_EQUALS("Wrong dimension names", "Qx", ws->getDimension(0)->getName());
    TSM_ASSERT_EQUALS("Wrong dimension names", "Qz", ws->getDimension(1)->getName());
    TSM_ASSERT_EQUALS("Wrong units on qx", "(Ang^-1)", ws->getDimension(0)->getUnits());
    TSM_ASSERT_EQUALS("Wrong units on qz", "(Ang^-1)", ws->getDimension(1)->getUnits());
    TSM_ASSERT_EQUALS("Wrong number of bins in qx", 10, ws->getDimension(0)->getNBins());
    TSM_ASSERT_EQUALS("Wrong number of bins in qy", 10, ws->getDimension(1)->getNBins());

    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_check_output_data()
  {
    std::string outWSName("ConvertToReflectometryQTest_OutputWS");

    doExecute(outWSName);

    IMDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName) );
    TS_ASSERT(ws);

    //Detailed checks associated with the output workspace.

    AnalysisDataService::Instance().remove(outWSName);
  }

};


#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQTEST_H_ */