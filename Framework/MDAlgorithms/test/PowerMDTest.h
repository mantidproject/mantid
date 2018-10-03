// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_POWERMDTEST_H_
#define MANTID_MDALGORITHMS_POWERMDTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/PowerMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class PowerMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    PowerMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_histo() {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("PowerMD", "histo", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 4.0, 1e-5);
  }

  void test_histo_with_Exponent() {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("PowerMD", "histo", "out", true,
                                             "Exponent", "-3.0");
    TS_ASSERT_DELTA(out->getSignalAt(0), 1. / 8., 1e-5);
  }

  void test_event_fails() {
    UnaryOperationMDTestHelper::doTest("PowerMD", "event", "out",
                                       false /* fails*/);
  }
};

#endif /* MANTID_MDALGORITHMS_POWERMDTEST_H_ */
