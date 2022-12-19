// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeROI.h"

using Mantid::Kernel::TimeROI;

class TimeROITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeROITest *createSuite() { return new TimeROITest(); }
  static void destroySuite(TimeROITest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};
