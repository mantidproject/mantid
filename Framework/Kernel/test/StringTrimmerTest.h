// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/StringTrimmer.h"

class StringTrimmerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StringTrimmerTest *createSuite() { return new StringTrimmerTest(); }
  static void destroySuite(StringTrimmerTest *suite) { delete suite; }

  void test_trimStringFromStart() {
    std::string testString = "   My Test String";
    Mantid::Kernel::trimStringFromStart(testString);
    TS_ASSERT_EQUALS(testString, "My Test String");
  }

  void test_trimStringFromEnd() {
    std::string testString = "My Test String   ";
    Mantid::Kernel::trimStringFromEnd(testString);
    TS_ASSERT_EQUALS(testString, "My Test String");
  }

  void test_trimString() {
    std::string testString = "  My Test String    ";
    Mantid::Kernel::trimString(testString);
    TS_ASSERT_EQUALS(testString, "My Test String");
  }
};
