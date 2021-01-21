// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/HistogramValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::HistogramValidator;

class HistogramValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramValidatorTest *createSuite() { return new HistogramValidatorTest(); }
  static void destroySuite(HistogramValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    HistogramValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_fail() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 10, 10);
    HistogramValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "The workspace must contain histogram data");
  }
};
