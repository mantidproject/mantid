#ifndef MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_
#define MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"

#include "SofQWTest.h"

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

  void test_exec()
  {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQWNormalisedPolygon>();

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
    TS_ASSERT_DELTA( result->readY(0)[1160], 22.8567683273, delta);
    TS_ASSERT_DELTA( result->readE(0)[1160], 0.2568965638, delta);

    TS_ASSERT_DELTA( result->readY(1)[1145], 7.5942160104, delta);
    TS_ASSERT_DELTA( result->readE(1)[1145], 0.1490079010 ,delta);

    TS_ASSERT_DELTA( result->readY(2)[1200], 2.0249626546, delta);
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.0752776593, delta);

    TS_ASSERT_DELTA( result->readY(3)[99], 0.0419939169, delta);
    TS_ASSERT_DELTA( result->readE(3)[99], 0.0175106375, delta);

    TS_ASSERT_DELTA( result->readY(4)[1654], 0.0167189448, delta);
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.0056801131, delta);

    TS_ASSERT_DELTA( result->readY(5)[1025], 0.0808168496, delta);
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.0161117732, delta);

  }
};


#endif /* MANTID_ALGORITHMS_SOFQW2TEST_H_ */
