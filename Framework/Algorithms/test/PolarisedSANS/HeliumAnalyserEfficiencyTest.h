// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/PolarisedSANS/HeliumAnalyserEfficiency.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class HeliumAnalyserEfficiencyTest : public CxxTest::TestSuite {
public:
  /**
   * @brief test name
   *
   */
  void testName() {
    HeliumAnalyserEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiency");
  }

  /**
   * @brief test init
   *
   */
  void testInit() {
    HeliumAnalyserEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }
};