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
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", in_ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TSM_ASSERT_THROWS("Not Implemented Yet", alg.execute(), std::runtime_error );
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

  void test_check_shouldnt_be_working()
  {
    std::string outWSName("ConvertToReflectometryQTest_OutputWS");

    doExecute(outWSName);
  }

};


#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQTEST_H_ */