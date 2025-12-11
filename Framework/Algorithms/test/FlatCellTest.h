// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FlatCell.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

/// Upda

class FlatCellTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(masker.name(), "FlatCell"); }

  void testVersion() { TS_ASSERT_EQUALS(masker.version(), 1); }

  void testCategory() { TS_ASSERT_EQUALS(masker.category(), "SANS"); }

  void testMean() {
    // Mantid::Algorithms::FlatCell fc;
    std::vector<double> v{1.0, 2.0, 3.0, 4.0, 5.0};
    TS_ASSERT_DELTA(masker.mean(v), 3, 1e-10);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT(masker.isInitialized());
  }

private:
  Mantid::Algorithms::FlatCell masker;
};
