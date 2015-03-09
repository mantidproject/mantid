#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_

#include "MantidMDAlgorithms/ReflectometryTransformKiKf.h"

#include <cxxtest/TestSuite.h>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

class ReflectometryTransformKiKfTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformKiKfTest *createSuite() { return new ReflectometryTransformKiKfTest(); }
  static void destroySuite( ReflectometryTransformKiKfTest *suite ) { delete suite; }

  void test_kimin_greater_than_kimax_throws()
  {
    double kiMin = 2;
    double kiMax = 1; //Smaller than kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }
  
  void test_kimin_equal_to_kimax_throws()
  {
    double kiMin = 1;
    double kiMax = 1; //Equal to kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }

  void test_kfmin_greater_than_kfmax_throws()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 2;
    double kfMax = 1; //Smaller than kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }
  
  void test_kfmin_equal_to_kfmax_throws()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 1; //Equal to kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }

  void test_incident_theta_negative()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 3; 
    double incidentTheta = -0.001; //Negative
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::out_of_range);
  }

  void test_incident_theta_too_large()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 3; 
    double incidentTheta = 90.001; //Too large
    TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::out_of_range);
  }

  void test_valid_construction_inputs()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS_NOTHING(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta));
  }

  void test_calulate_k()
  {
    const double wavelength = 1;

    //Sine 0 = 0
    CalculateReflectometryK A(0);
    TS_ASSERT_EQUALS(0, A.execute(wavelength));

    //Sine 90 = 1
    CalculateReflectometryK B(90);
    TS_ASSERT_DELTA(2*M_PI/wavelength, B.execute(wavelength), 0.0001);

    //Sine 270 = -1
    CalculateReflectometryK C(270);
    TS_ASSERT_DELTA(-2*M_PI/wavelength, C.execute(wavelength), 0.0001);
  }

  void test_recalculate_k()
  {
    const double wavelength = 1;

    CalculateReflectometryK A(90);
    TS_ASSERT_DELTA(2*M_PI/wavelength, A.execute(wavelength), 0.0001);

    //Now re-execute on the same calculation object.
    TS_ASSERT_DELTA(M_PI/wavelength, A.execute(2*wavelength), 0.0001);
  }


};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMKIKFTEST_H_ */
