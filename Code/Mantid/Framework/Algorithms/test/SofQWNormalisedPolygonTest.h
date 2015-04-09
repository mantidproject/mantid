#ifndef MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_
#define MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

#include <iomanip>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class SofQWNormalisedPolygonTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_Aliased_To_SofQW3()
  {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_EQUALS("SofQW3", alg.alias())
  }

  void xtest_exec()
  {
    SofQWNormalisedPolygon sqw;
    sqw.initialize();

    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","IRS26173_ipg.nxs");
    const std::string inputWS = "inputWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    Mantid::API::MatrixWorkspace_sptr inWS;
    TS_ASSERT_THROWS_NOTHING( inWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(inputWS)) );
    WorkspaceHelpers::makeDistribution(inWS);

    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("InputWorkspace",inputWS) );
    const std::string outputWS = "result";
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("QAxisBinning","0.5,0.25,2") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EMode","Indirect") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EFixed","1.84") );

    TS_ASSERT_THROWS_NOTHING( sqw.execute() );
    TS_ASSERT( sqw.isExecuted() );

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );

    TS_ASSERT_EQUALS( result->getAxis(0)->length(), 1904 );
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "DeltaE" );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(0), -0.5590, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(999), -0.0971, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(1900), 0.5728, 0.0001 );

    TS_ASSERT_EQUALS( result->getAxis(1)->length(), 7 );
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), 0.5 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(3), 1.25 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(6), 2.0 );

    const double delta(1e-08);
    TS_ASSERT_DELTA( result->readY(0)[1160], 17.5583314826, delta);
    TS_ASSERT_DELTA( result->readE(0)[1160], 0.197345265992, delta);

    TS_ASSERT_DELTA( result->readY(1)[1145], 4.61301046588, delta);
    TS_ASSERT_DELTA( result->readE(1)[1145], 0.0721823446635,delta);

    TS_ASSERT_DELTA( result->readY(2)[1200], 1.33394133548, delta);
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.0419839252961, delta);

    TS_ASSERT_DELTA( result->readY(3)[99], 0.0446085388561, delta);
    TS_ASSERT_DELTA( result->readE(3)[99], 0.0185049423467, delta);

    TS_ASSERT_DELTA( result->readY(4)[1654], 0.0171136490957 , delta);
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.005007299861, delta);

    TS_ASSERT_DELTA( result->readY(5)[1025], 0.0516113202152, delta);
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.0102893133461, delta);

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);    
  }
};


#endif /* MANTID_ALGORITHMS_SOFQW2TEST_H_ */

