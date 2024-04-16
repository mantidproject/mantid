// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"

namespace {} // namespace

using Mantid::Algorithms::FlipperEfficiency;

class FlipperEfficiencyTest : public CxxTest::TestSuite {
public:
  void test_name() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.name(), "FlipperEfficiency");
  }

  void test_version() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_category() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.category(), "SANS\\PolarizationCorrections");
  }
};
