// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/FormattingHelpers.h"
#include <cxxtest/TestSuite.h>

class FormattingHelpersTest : public CxxTest::TestSuite {
public:
  static FormattingHelpersTest *createSuite() { return new FormattingHelpersTest(); }
  static void destroySuite(FormattingHelpersTest *suite) { delete suite; }

  void testFormatDouble() {
    TS_ASSERT_EQUALS(Mantid::DataHandling::formatDouble(1.0), "1.0");
    TS_ASSERT_EQUALS(Mantid::DataHandling::formatDouble(0.0), "0.0");
    TS_ASSERT_EQUALS(Mantid::DataHandling::formatDouble(4.25), "4.25");
    TS_ASSERT_EQUALS(Mantid::DataHandling::formatDouble(2.0 + 5e-13), "2.0");
    TS_ASSERT_EQUALS(Mantid::DataHandling::formatDouble(-5.0), "-5.0");
  }
};
