#ifndef SOFQWCENTRETEST_H_
#define SOFQWCENTRETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQWCentre.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

#include "SofQWTest.h"

using namespace Mantid::API;

class SofQWCentreTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    Mantid::Algorithms::SofQWCentre sqw;
    TS_ASSERT_EQUALS( sqw.name(), "SofQWCentre" );
  }

  void testVersion()
  {
    Mantid::Algorithms::SofQWCentre sqw;
    TS_ASSERT_EQUALS( sqw.version(), 1 );
  }

  void testCategory()
  {
    Mantid::Algorithms::SofQWCentre sqw;
    TS_ASSERT_EQUALS( sqw.category(), "Inelastic" );
  }

  void testInit()
  {
    Mantid::Algorithms::SofQWCentre sqw;
    TS_ASSERT_THROWS_NOTHING( sqw.initialize() );
    TS_ASSERT( sqw.isInitialized() );
  }

  void testExec()
  {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQWCentre>();

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
    TS_ASSERT_DELTA( result->readY(0)[1160], 54.85624399, delta);
    TS_ASSERT_DELTA( result->readE(0)[1160], 0.34252858, delta);
    TS_ASSERT_DELTA( result->readY(1)[1145], 22.72491806, delta);
    TS_ASSERT_DELTA( result->readE(1)[1145], 0.19867742, delta);
    TS_ASSERT_DELTA( result->readY(2)[1200], 6.76047436, delta);
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.10863549, delta);
    TS_ASSERT_DELTA( result->readY(3)[99], 0.16439574, delta);
    TS_ASSERT_DELTA( result->readE(3)[99], 0.03414360, delta);
    TS_ASSERT_DELTA( result->readY(4)[1654], 0.069311442, delta);
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.007573484, delta);
    TS_ASSERT_DELTA( result->readY(5)[1025], 0.226287179, delta);
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.02148236, delta);

  }

};

#endif /*SOFQWTEST_H_*/
