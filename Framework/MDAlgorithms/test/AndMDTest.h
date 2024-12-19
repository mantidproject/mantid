// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidFrameworkTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDAlgorithms/AndMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class AndMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    AndMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_histo_histo() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "histo_zero", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_histo_histo_masked() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "histo_masked", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_masked", "histo_masked", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_scalar_or_event_fails() {
    BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("AndMD", "event_A", "event_B", "out", false /*fails*/);
  }
};
