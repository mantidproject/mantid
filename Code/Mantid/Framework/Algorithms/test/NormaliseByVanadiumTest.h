#ifndef MANTID_ALGORITHMS_NORMALISEBYVANADIUMTEST_H_
#define MANTID_ALGORITHMS_NORMALISEBYVANADIUMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <iostream>
#include <iomanip>
#include <boost/shared_ptr.hpp>

#include "MantidAlgorithms/NormaliseByVanadium.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class NormaliseByVanadiumTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByVanadiumTest *createSuite() { return new NormaliseByVanadiumTest(); }
  static void destroySuite( NormaliseByVanadiumTest *suite ) { delete suite; }


  void testNoSampleWorkspace()
  {
    //MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    NormaliseByVanadium alg;
    alg.initialize();
    //alg.setProperty("SampleInputWorkspace", sampleWS);
    alg.setProperty("VanadiumInputWorkspace", vanadiumWS);
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TS_ASSERT(!alg.validateProperties());

  }

  void testNoVanadiumWorkspace()
  {
    MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    //MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    NormaliseByVanadium alg;
    alg.initialize();
    alg.setProperty("SampleInputWorkspace", sampleWS);
    //alg.setProperty("VanadiumInputWorkspace", vanadiumWS);
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TS_ASSERT(!alg.validateProperties());
  }

  void testValidProperties()
  {
    MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    
    NormaliseByVanadium alg;
    alg.initialize();
    alg.setProperty("SampleInputWorkspace", sampleWS);
    alg.setProperty("VanadiumInputWorkspace", vanadiumWS);
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TS_ASSERT(alg.validateProperties());
  }

  void testExecution()
  {
    using Mantid::API::AnalysisDataService;

    MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::Create2DWorkspace(3, 10);
    MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::Create2DWorkspace(3, 10);
    
    NormaliseByVanadium alg;
    alg.initialize();
    alg.setProperty("SampleInputWorkspace", sampleWS);
    alg.setProperty("VanadiumInputWorkspace", vanadiumWS);
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(AnalysisDataService::Instance().doesExist("OutWS"));

    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("OutWS"));
    TS_ASSERT(NULL != result.get());

  }


};


#endif /* MANTID_ALGORITHMS_NORMALISEBYVANADIUMTEST_H_ */

