// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarisedSANS/SANSCalcDepolarisedAnalyserTransmission.h"

using Mantid::Algorithms::SANSCalcDepolarisedAnalyserTransmission;

class SANSCalcDepolarisedAnalyserTransmissionTest : public CxxTest::TestSuite {
public:
  void test_name() {
    Mantid::Algorithms::SANSCalcDepolarisedAnalyserTransmission const alg;
    TS_ASSERT_EQUALS(alg.name(), "SANSCalcDepolarisedAnalyserTransmission");
  }

  void test_version() {
    Mantid::Algorithms::SANSCalcDepolarisedAnalyserTransmission const alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_summary() {
    Mantid::Algorithms::SANSCalcDepolarisedAnalyserTransmission const alg;
    TS_ASSERT_EQUALS(alg.summary(), "test")
  }
};
