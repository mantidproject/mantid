// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RawCountValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::RawCountValidator;

class RawCountValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RawCountValidatorTest *createSuite() { return new RawCountValidatorTest(); }
  static void destroySuite(RawCountValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    RawCountValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_fail() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    ws->setDistribution(true);
    RawCountValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "A workspace containing numbers of counts is required here");
  }
};
