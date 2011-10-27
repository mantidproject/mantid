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


//=====================================================================================
// Functional tests
//=====================================================================================
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

    MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(50, 10);
    MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(50, 10); //Effectively normalisation by itself.
    
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
    TSM_ASSERT_EQUALS("Number of histograms does not match between sample and normalised by vanadium sample", sampleWS->getNumberHistograms(), result->getNumberHistograms());
    TS_ASSERT_EQUALS(sampleWS->size(), result->size());
    
    //Could also run compare workspace alg here!
    for(int i = 0; i < result->getNumberHistograms(); i++)
    {
      for(int j = 0; j < sampleWS->readY(i).size(); j++)
      {
        TS_ASSERT(sampleWS->readY(i)[j] == result->readY(i)[j]);
      }
    }

  }
};

//=====================================================================================
// Peformance tests
//=====================================================================================
class NormaliseByVanadiumTestPeformance : public CxxTest::TestSuite
{
public:

  void testExecution()
  {
    using Mantid::API::AnalysisDataService;

    MatrixWorkspace_sptr sampleWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(200, 10);
    MatrixWorkspace_sptr vanadiumWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(200, 10); //Effectively normalisation by itself.
    
    NormaliseByVanadium alg;
    alg.initialize();
    alg.setProperty("SampleInputWorkspace", sampleWS);
    alg.setProperty("VanadiumInputWorkspace", vanadiumWS);
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(AnalysisDataService::Instance().doesExist("OutWS"));
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("OutWS"));
  }
};

#endif /* MANTID_ALGORITHMS_NORMALISEBYVANADIUMTEST_H_ */

