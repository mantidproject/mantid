// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::WorkspaceUnitValidator;

class WorkspaceUnitValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceUnitValidatorTest *createSuite() { return new WorkspaceUnitValidatorTest(); }
  static void destroySuite(WorkspaceUnitValidatorTest *suite) { delete suite; }

  void test_fail() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    WorkspaceUnitValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "The workspace must have units");
  }

  void test_success() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    ws->getAxis(0)->setUnit("TOF");
    WorkspaceUnitValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }
};
