// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/NotMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class NotMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    NotMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_histo() {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("NotMD", "histo", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_event_fails() { UnaryOperationMDTestHelper::doTest("NotMD", "event", "out", false /* fails*/); }
};
