// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_WORKSPACEHASDXVALIDATORTEST_H_
#define MANTID_API_WORKSPACEHASDXVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceHasDxValidator.h"

#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;

class WorkspaceHasDxValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceHasDxValidatorTest *createSuite() {
    return new WorkspaceHasDxValidatorTest();
  }
  static void destroySuite(WorkspaceHasDxValidatorTest *suite) { delete suite; }

  void test_returns_empty_string_for_valid_workspaces() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    auto dx = Kernel::make_cow<HistogramData::HistogramDx>(1, 0.);
    ws->setSharedDx(0, std::move(dx));
    WorkspaceHasDxValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "")
  }

  void test_returns_message_for_invalid_workspaces() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    WorkspaceHasDxValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws),
                     "The workspace must have Dx values set")
  }
};

#endif /* MANTID_API_WORKSPACEHASDXVALIDATORTEST_H_ */
