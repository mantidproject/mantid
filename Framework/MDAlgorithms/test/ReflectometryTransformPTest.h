#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ReflectometryTransformP.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class ReflectometryTransformPTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformPTest *createSuite() {
    return new ReflectometryTransformPTest();
  }
  static void destroySuite(ReflectometryTransformPTest *suite) { delete suite; }

  void test_kimin_greater_than_kimax_throws() {
    double kiMin = 2;
    double kiMax = 1; // Smaller than kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::invalid_argument);
  }

  void test_kimin_equal_to_kimax_throws() {
    double kiMin = 1;
    double kiMax = 1; // Equal to kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::invalid_argument);
  }

  void test_kfmin_greater_than_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 2;
    double kfMax = 1; // Smaller than kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::invalid_argument);
  }

  void test_kfmin_equal_to_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 1; // Equal to kfMin!
    double incidentTheta = 1;
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::invalid_argument);
  }

  void test_incident_theta_negative() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = -0.001; // Negative
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::out_of_range);
  }

  void test_incident_theta_too_large() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = 90.001; // Too large
    TS_ASSERT_THROWS(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta),
        std::out_of_range);
  }

  void test_valid_construction_inputs() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    TS_ASSERT_THROWS_NOTHING(
        ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta));
  }

  void test_calulate_diff_p() {
    const double wavelength = 1;

    CalculateReflectometryP A;
    A.setThetaIncident(0);
    A.setThetaFinal(0);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));

    CalculateReflectometryP B;
    B.setThetaIncident(90);
    B.setThetaFinal(0);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim1(wavelength), 0.0001);

    CalculateReflectometryP C;
    C.setThetaIncident(0);
    C.setThetaFinal(90);
    TS_ASSERT_DELTA(-2 * M_PI / wavelength, C.calculateDim1(wavelength),
                    0.0001);

    CalculateReflectometryP D;
    D.setThetaIncident(90);
    D.setThetaFinal(90);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));
  }

  void test_calulate_sum_p() {
    const double wavelength = 1;

    CalculateReflectometryP A;
    A.setThetaIncident(0);
    A.setThetaFinal(0);
    TS_ASSERT_EQUALS(0, A.calculateDim0(wavelength));

    CalculateReflectometryP B;
    B.setThetaIncident(90);
    B.setThetaFinal(0);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP C;
    C.setThetaIncident(0);
    C.setThetaFinal(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, C.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP D;
    D.setThetaIncident(90);
    D.setThetaFinal(90);
    TS_ASSERT_DELTA(4 * M_PI / wavelength, D.calculateDim0(wavelength), 0.0001);
  }
};

#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_ */
