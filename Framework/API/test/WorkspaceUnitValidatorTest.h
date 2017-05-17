#ifndef MANTID_API_WORKSPACEUNITVALIDATORTEST_H_
#define MANTID_API_WORKSPACEUNITVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::WorkspaceUnitValidator;

class WorkspaceUnitValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceUnitValidatorTest *createSuite() {
    return new WorkspaceUnitValidatorTest();
  }
  static void destroySuite(WorkspaceUnitValidatorTest *suite) { delete suite; }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    WorkspaceUnitValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "The workspace must have units");
  }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    ws->getAxis(0)->setUnit("TOF");
    WorkspaceUnitValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }
};

#endif /* MANTID_API_WORKSPACEUNITVALIDATORTEST_H_ */
