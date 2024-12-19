// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include "MantidReflectometry/ReflectometryTransformP.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Reflectometry;
using namespace Mantid::API;

class ReflectometryTransformPTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformPTest *createSuite() { return new ReflectometryTransformPTest(); }
  static void destroySuite(ReflectometryTransformPTest *suite) { delete suite; }

  void test_kimin_greater_than_kimax_throws() {
    double kiMin = 2;
    double kiMax = 1; // Smaller than kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kimin_equal_to_kimax_throws() {
    double kiMin = 1;
    double kiMax = 1; // Equal to kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kfmin_greater_than_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 2;
    double kfMax = 1; // Smaller than kfMin!
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kfmin_equal_to_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 1; // Equal to kfMin!
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_incident_theta_negative() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = -0.001; // Negative
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::out_of_range &);
  }

  void test_incident_theta_too_large() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = 90.001; // Too large
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::out_of_range &);
  }

  void test_valid_construction_inputs() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS_NOTHING(ReflectometryTransformP(kiMin, kiMax, kfMin, kfMax, incidentTheta, version));
  }

  void test_calulate_diff_p_v1() {
    // In v1, thetaFinal is set equal to the given twoTheta
    const double wavelength = 1;
    const int version = 1;

    CalculateReflectometryP A(version);
    A.setThetaIncident(0);
    A.setTwoTheta(0);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));

    CalculateReflectometryP B(version);
    B.setThetaIncident(90);
    B.setTwoTheta(0);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim1(wavelength), 0.0001);

    CalculateReflectometryP C(version);
    C.setThetaIncident(0);
    C.setTwoTheta(90);
    TS_ASSERT_DELTA(-2 * M_PI / wavelength, C.calculateDim1(wavelength), 0.0001);

    CalculateReflectometryP D(version);
    D.setThetaIncident(90);
    D.setTwoTheta(90);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));
  }

  void test_calulate_diff_p_v2() {
    // In v2, thetaFinal is set from twoTheta - thetaIncident
    const double wavelength = 1;
    const int version = 2;

    CalculateReflectometryP A(version);
    A.setThetaIncident(0);
    A.setTwoTheta(0);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));

    CalculateReflectometryP B(version);
    B.setThetaIncident(90);
    B.setTwoTheta(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim1(wavelength), 0.0001);

    CalculateReflectometryP C(version);
    C.setThetaIncident(0);
    C.setTwoTheta(90);
    TS_ASSERT_DELTA(-2 * M_PI / wavelength, C.calculateDim1(wavelength), 0.0001);

    CalculateReflectometryP D(version);
    D.setThetaIncident(90);
    D.setTwoTheta(180);
    TS_ASSERT_EQUALS(0, A.calculateDim1(wavelength));
  }

  void test_calulate_sum_p_v1() {
    // In v1, thetaFinal is set equal to the given twoTheta
    const double wavelength = 1;
    const int version = 1;

    CalculateReflectometryP A(version);
    A.setThetaIncident(0);
    A.setTwoTheta(0);
    TS_ASSERT_EQUALS(0, A.calculateDim0(wavelength));

    CalculateReflectometryP B(version);
    B.setThetaIncident(90);
    B.setTwoTheta(0);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP C(version);
    C.setThetaIncident(0);
    C.setTwoTheta(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, C.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP D(version);
    D.setThetaIncident(90);
    D.setTwoTheta(90);
    TS_ASSERT_DELTA(4 * M_PI / wavelength, D.calculateDim0(wavelength), 0.0001);
  }

  void test_calulate_sum_p_v2() {
    // In v2, thetaFinal is set from twoTheta - thetaIncident
    const double wavelength = 1;
    const int version = 2;

    CalculateReflectometryP A(version);
    A.setThetaIncident(0);
    A.setTwoTheta(0);
    TS_ASSERT_EQUALS(0, A.calculateDim0(wavelength));

    CalculateReflectometryP B(version);
    B.setThetaIncident(90);
    B.setTwoTheta(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP C(version);
    C.setThetaIncident(0);
    C.setTwoTheta(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, C.calculateDim0(wavelength), 0.0001);

    CalculateReflectometryP D(version);
    D.setThetaIncident(90);
    D.setTwoTheta(180);
    TS_ASSERT_DELTA(4 * M_PI / wavelength, D.calculateDim0(wavelength), 0.0001);
  }

private:
  std::array<int, 2> m_versions{1, 2};
};
