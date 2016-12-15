#ifndef CALCULATESLITSTEST_H_
#define CALCULATESLITSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateSlits.h"

using namespace Mantid::Algorithms;

class CalculateSlitsTest : public CxxTest::TestSuite {
public:

  void testSensibleArgs() {
    CalculateSlits alg;
    alg.initialize();
    alg.setProperty("Slit1Slit2", 1940.5);
    alg.setProperty("Slit2SA", 364.);
    alg.setProperty("Angle", 0.7);
    alg.setProperty("Footprint", 50.);
    alg.setProperty("Resolution", 0.03);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double slit1 = alg.getProperty("Slit1");
    TS_ASSERT_EQUALS(slit1, 1.0784367635946033);

    const double slit2 = alg.getProperty("Slit2");
    TS_ASSERT_EQUALS(slit2, 0.34402409376933002);
  }

  void testWithNegativeAngle() {
    CalculateSlits alg;
    alg.initialize();
    alg.setProperty("Slit1Slit2", 1940.5);
    alg.setProperty("Slit2SA", 364.);
    alg.setProperty("Angle", -0.7);
    alg.setProperty("Footprint", 50.);
    alg.setProperty("Resolution", 0.03);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double slit1 = alg.getProperty("Slit1");
    TS_ASSERT_EQUALS(slit1, -1.0784367635946033);

    const double slit2 = alg.getProperty("Slit2");
    TS_ASSERT_EQUALS(slit2, -0.34402409376933002);
  }

  void testWithZeroAngle() {
    CalculateSlits alg;
    alg.initialize();
    alg.setProperty("Slit1Slit2", 1940.5);
    alg.setProperty("Slit2SA", 364.);
    alg.setProperty("Angle", 0.);
    alg.setProperty("Footprint", 50.);
    alg.setProperty("Resolution", 0.03);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double slit1 = alg.getProperty("Slit1");
    TS_ASSERT_EQUALS(slit1, 0.0);

    const double slit2 = alg.getProperty("Slit2");
    TS_ASSERT_EQUALS(slit2, 0.0);
  }

  void testWithNanAndInf() {
    const double nan= std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const double ninf = -inf;

    CalculateSlits alg;
    alg.initialize();
    alg.setProperty("Slit1Slit2", nan);
    alg.setProperty("Slit2SA", nan);
    alg.setProperty("Angle", inf);
    alg.setProperty("Footprint", inf);
    alg.setProperty("Resolution", ninf);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double slit1 = alg.getProperty("Slit1");
    TS_ASSERT(isnan(slit1));

    const double slit2 = alg.getProperty("Slit2");
    TS_ASSERT(isnan(slit2));
  }

  void testWithNoArgs() {
    CalculateSlits alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double slit1 = alg.getProperty("Slit1");
    TS_ASSERT(isnan(slit1));

    const double slit2 = alg.getProperty("Slit2");
    TS_ASSERT(isnan(slit2));
  }

};


#endif /*CALCULATESLITSTEST_H_*/
