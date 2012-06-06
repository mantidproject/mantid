#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_

#define PI 3.14159265

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/ReflectometryTransformKiKf.h"


using namespace Mantid::MDEvents;

class ReflectometryTransformKiKfTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformKiKfTest *createSuite() { return new ReflectometryTransformKiKfTest(); }
  static void destroySuite( ReflectometryTransformKiKfTest *suite ) { delete suite; }


  void test_calulate_k()
  {
    const double wavelength = 1;

    CalculateReflectometryK A(0);
    TS_ASSERT_EQUALS(0, A.execute(wavelength));

    CalculateReflectometryK B(90);
    TS_ASSERT_DELTA(2*PI/wavelength, B.execute(wavelength), 0.0001);

    CalculateReflectometryK C(270);
    TS_ASSERT_DELTA(-2*PI/wavelength, C.execute(wavelength), 0.0001);
  }

  void test_recalculate_k()
  {
    const double wavelength = 1;

    CalculateReflectometryK A(90);
    TS_ASSERT_DELTA(2*PI/wavelength, A.execute(wavelength), 0.0001);

    TS_ASSERT_DELTA(PI/wavelength, A.execute(2*wavelength), 0.0001);
  }


};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_ */