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

  void test_given_explicit_unit_when_check_is_valid_that_workspace_is_valid() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    ws->getAxis(0)->setUnit("TOF");
    WorkspaceUnitValidator validator("TOF");
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void
  test_given_unitless_workspace_when_check_is_valid_that_workspace_is_not_valid() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    WorkspaceUnitValidator validator("TOF");
    TS_ASSERT_EQUALS(validator.isValid(ws),
                     "The workspace must have units of TOF");
  }

  void test_given_multiple_units_when_check_is_valid_that_workspace_is_valid() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    ws->getAxis(0)->setUnit("dSpacing");
    std::vector<std::string> unitIDs = {"TOF", "dSpacing"};
    WorkspaceUnitValidator validator(unitIDs);
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void
  test_given_multiple_units_and_unitless_workspace_when_check_is_valid_that_workspace_is_not_valid() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    std::vector<std::string> unitIDs = {"TOF", "dSpacing"};
    WorkspaceUnitValidator validator(unitIDs);
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "The workspace must have one of the following units: TOF, dSpacing");
  }
};

#endif /* MANTID_API_WORKSPACEUNITVALIDATORTEST_H_ */
