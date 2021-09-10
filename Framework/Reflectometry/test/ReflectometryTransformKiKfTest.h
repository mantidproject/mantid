// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/ReflectometryTransformKiKf.h"

#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Reflectometry;

class ReflectometryTransformKiKfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformKiKfTest *createSuite() { return new ReflectometryTransformKiKfTest(); }
  static void destroySuite(ReflectometryTransformKiKfTest *suite) { delete suite; }

  void test_kimin_greater_than_kimax_throws() {
    double kiMin = 2;
    double kiMax = 1; // Smaller than kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kimin_equal_to_kimax_throws() {
    double kiMin = 1;
    double kiMax = 1; // Equal to kiMin!
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kfmin_greater_than_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 2;
    double kfMax = 1; // Smaller than kfMin!
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_kfmin_equal_to_kfmax_throws() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 1; // Equal to kfMin!
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::invalid_argument &);
  }

  void test_incident_theta_negative() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = -0.001; // Negative
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::out_of_range &);
  }

  void test_incident_theta_too_large() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 3;
    double incidentTheta = 90.001; // Too large
    for (auto version : m_versions)
      TS_ASSERT_THROWS(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version),
                       const std::out_of_range &);
  }

  void test_valid_construction_inputs() {
    double kiMin = 1;
    double kiMax = 2;
    double kfMin = 1;
    double kfMax = 2;
    double incidentTheta = 1;
    for (auto version : m_versions)
      TS_ASSERT_THROWS_NOTHING(ReflectometryTransformKiKf(kiMin, kiMax, kfMin, kfMax, incidentTheta, version));
  }

  void test_calulate_k_v1() {
    const double wavelength = 1;
    const int version = 1;

    // Sine 0 = 0

    CalculateReflectometryKiKf A(version);
    A.setThetaIncident(0);
    TS_ASSERT_EQUALS(0, A.calculateDim0(wavelength));

    // Sine 90 = 1
    CalculateReflectometryKiKf B(version);
    B.setThetaIncident(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim0(wavelength), 0.0001);

    // Sine 270 = -1
    CalculateReflectometryKiKf C(version);
    C.setThetaIncident(270);
    TS_ASSERT_DELTA(-2 * M_PI / wavelength, C.calculateDim0(wavelength), 0.0001);
  }

  void test_calulate_k_v2() {
    const double wavelength = 1;
    const int version = 2;

    // Sine 0 = 0

    CalculateReflectometryKiKf A(version);
    A.setThetaIncident(0);
    TS_ASSERT_EQUALS(0, A.calculateDim0(wavelength));

    // Sine 90 = 1
    CalculateReflectometryKiKf B(version);
    B.setThetaIncident(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, B.calculateDim0(wavelength), 0.0001);

    // Sine 270 = -1
    CalculateReflectometryKiKf C(version);
    C.setThetaIncident(270);
    TS_ASSERT_DELTA(-2 * M_PI / wavelength, C.calculateDim0(wavelength), 0.0001);
  }

  void test_recalculate_k_v1() {
    const double wavelength = 1;
    const int version = 1;

    CalculateReflectometryKiKf A(version);
    A.setThetaIncident(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, A.calculateDim0(wavelength), 0.0001);

    // Now re-execute on the same calculation object.
    TS_ASSERT_DELTA(M_PI / wavelength, A.calculateDim0(2 * wavelength), 0.0001);
  }

  void test_recalculate_k_v2() {
    const double wavelength = 1;
    const int version = 2;

    CalculateReflectometryKiKf A(version);
    A.setThetaIncident(90);
    TS_ASSERT_DELTA(2 * M_PI / wavelength, A.calculateDim0(wavelength), 0.0001);

    // Now re-execute on the same calculation object.
    TS_ASSERT_DELTA(M_PI / wavelength, A.calculateDim0(2 * wavelength), 0.0001);
  }

private:
  std::array<int, 2> m_versions{1, 2};
};
