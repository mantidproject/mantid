#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidMDAlgorithms/ReflectometryTransformP.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class ReflectometryTransformPTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformPTest *createSuite() { return new ReflectometryTransformPTest(); }
  static void destroySuite( ReflectometryTransformPTest *suite ) { delete suite; }


  void test_kimin_greater_than_kimax_throws()
  {
    double kiMin = 2;
    double kiMax = 1; //Smaller than kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }
  
  void test_kimin_equal_to_kimax_throws()
  {
    double kiMin = 1;
    double kiMax = 1; //Equal to kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }

  void test_kfmin_greater_than_kfmax_throws()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 2;
    double kfMax = 1; //Smaller than kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }
  
  void test_kfmin_equal_to_kfmax_throws()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 1; //Equal to kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::invalid_argument);
  }

  void test_incident_theta_negative()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 3; 
    double incidentTheta = -0.001; //Negative
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::out_of_range);
  }

  void test_incident_theta_too_large()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 3; 
    double incidentTheta = 90.001; //Too large
    TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta), std::out_of_range);
  }

  void test_valid_construction_inputs()
  {
    double kiMin = 1;
    double kiMax = 2; 
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS_NOTHING(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta));
  }

  void test_calulate_diff_p()
  {
    const double wavelength = 1;

    CalculateReflectometryDiffP A(0);
    A.setThetaFinal(0);
    TS_ASSERT_EQUALS(0, A.execute(wavelength));

    CalculateReflectometryDiffP B(90);
    B.setThetaFinal(0);
    TS_ASSERT_DELTA(2*M_PI/wavelength, B.execute(wavelength), 0.0001);

    CalculateReflectometryDiffP C(0);
    C.setThetaFinal(90);
    TS_ASSERT_DELTA(-2*M_PI/wavelength, C.execute(wavelength), 0.0001);

    CalculateReflectometryDiffP D(90);
    D.setThetaFinal(90);
    TS_ASSERT_EQUALS(0, A.execute(wavelength));
  }

  void test_calulate_sum_p()
  {
    const double wavelength = 1;

    CalculateReflectometrySumP A(0);
    A.setThetaFinal(0);
    TS_ASSERT_EQUALS(0, A.execute(wavelength));

    CalculateReflectometrySumP B(90);
    B.setThetaFinal(0);
    TS_ASSERT_DELTA(2*M_PI/wavelength, B.execute(wavelength), 0.0001);

    CalculateReflectometrySumP C(0);
    C.setThetaFinal(90);
    TS_ASSERT_DELTA(2*M_PI/wavelength, C.execute(wavelength), 0.0001);

    CalculateReflectometrySumP D(90);
    D.setThetaFinal(90);
    TS_ASSERT_DELTA(4*M_PI/wavelength, D.execute(wavelength), 0.0001);
  }

};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_ */
