// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;

class HardThresholdBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HardThresholdBackgroundTest *createSuite() { return new HardThresholdBackgroundTest(); }
  static void destroySuite(HardThresholdBackgroundTest *suite) { delete suite; }

  void test_isBackground() {
    const double threshold = 1;
    MDHistoWorkspace_sptr ws = makeFakeMDHistoWorkspace(threshold, 1, 1);
    auto iterator = ws->createIterator(nullptr);

    HardThresholdBackground strategy(threshold, Mantid::API::NoNormalization);

    TS_ASSERT(strategy.isBackground(iterator.get()));
  }
};
