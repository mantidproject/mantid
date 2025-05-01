// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include <json/value.h>
#include <memory>

// Property implementations
#include "MantidAPI/WorkspaceProperty.hxx"
#include "MantidKernel/PropertyWithValue.hxx"

using Mantid::MantidVec;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;

class WorkspacePropertyTest : public CxxTest::TestSuite {

  class WorkspaceTester1 : public WorkspaceTester {
  public:
    const std::string id() const override { return "WorkspacePropTest"; }
  };

  // Second, identical private test class - used for testing check on workspace
  // type in isValid()
  class WorkspaceTester2 : public WorkspaceTester {
  public:
    const std::string id() const override { return "WorkspacePropTest"; }
  };

  using WorkspacePropertyWorkspace = WorkspaceProperty<Workspace>;
  using WorkspacePropertyWorkspace_uptr = std::unique_ptr<WorkspacePropertyWorkspace>;

  using WorkspacePropertyWorkspaceTester2 = WorkspaceProperty<WorkspaceTester2>;
  using WorkspacePropertyWorkspaceTester2_uptr = std::unique_ptr<WorkspacePropertyWorkspaceTester2>;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspacePropertyTest *createSuite() { return new WorkspacePropertyTest(); }
  static void destroySuite(WorkspacePropertyTest *suite) { delete suite; }

  WorkspacePropertyTest() {
    AnalysisDataService::Instance().clear();
    WorkspaceFactory::Instance().subscribe<WorkspaceTester1>("WorkspacePropertyTest");
    WorkspaceFactory::Instance().subscribe<WorkspaceTester2>("WorkspacePropertyTest2");
    wsp1 = std::make_unique<WorkspacePropertyWorkspace>("workspace1", "ws1", Direction::Input);
    wsp2 = std::make_unique<WorkspacePropertyWorkspace>("workspace2", "", Direction::Output);
    wsp3 = std::make_unique<WorkspacePropertyWorkspaceTester2>("workspace3", "ws3", Direction::InOut);
    // Two optional properties of different types
    wsp4 = std::make_unique<WorkspacePropertyWorkspace>("workspace4", "", Direction::Input, PropertyMode::Optional);
    wsp5 =
        std::make_unique<WorkspacePropertyWorkspaceTester2>("workspace5", "", Direction::Input, PropertyMode::Optional);
    wsp6 = std::make_unique<WorkspacePropertyWorkspace>("InvalidNameTest", "", Direction::Output);
  }

  void testConstructor() { TS_ASSERT_THROWS(WorkspaceProperty<Workspace>("test", "", 3), const std::out_of_range &) }

  void testValue() {
    TS_ASSERT_EQUALS(wsp1->value(), "ws1")
    TS_ASSERT_EQUALS(wsp2->value(), "")
    TS_ASSERT_EQUALS(wsp3->value(), "ws3")
  }

  void testValueAsJson() {
    TS_ASSERT_EQUALS("ws1", wsp1->valueAsJson().asString());
    TS_ASSERT_EQUALS("", wsp2->valueAsJson().asString());
    TS_ASSERT_EQUALS("ws3", wsp3->valueAsJson().asString());
  }

  void testIsValueSerializable() {
    WorkspaceProperty<Workspace> p("PropertyName", "", Direction::InOut);
    TS_ASSERT(p.isValueSerializable());
    p.setValue("WorkspaceName");
    TS_ASSERT(p.isValueSerializable());
    p.setValue("");
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    p.setDataItem(ws);
    TS_ASSERT(!p.isDefault())
    TS_ASSERT(!p.isValueSerializable());
  }

  void testSetValue() {
    TS_ASSERT_EQUALS(wsp1->setValue(""), "Enter a name for the Input/InOut workspace")
    TS_ASSERT_EQUALS(wsp1->value(), "")
    TS_ASSERT_EQUALS(wsp1->setValueFromJson(Json::Value("")), "Enter a name for the Input/InOut workspace")
    TS_ASSERT_EQUALS(wsp1->value(), "")

    TS_ASSERT_EQUALS(wsp1->setValue("newValue"), "Workspace \"newValue\" was not found in the Analysis Data Service")
    TS_ASSERT_EQUALS(wsp1->value(), "newValue")
    TS_ASSERT_EQUALS(wsp1->setValueFromJson(Json::Value("newValue")),
                     "Workspace \"newValue\" was not found in the Analysis Data Service")
    TS_ASSERT_EQUALS(wsp1->value(), "newValue")
    wsp1->setValue("ws1");
  }

  void testSetValue_On_Optional() {
    TS_ASSERT_EQUALS(wsp4->setValue(""), "");
    TS_ASSERT_EQUALS(wsp4->value(), "");
    TS_ASSERT_EQUALS(wsp4->setValue("newValue"), "Workspace \"newValue\" was not found in the Analysis Data Service");
    TS_ASSERT_EQUALS(wsp4->value(), "newValue");
    wsp4->setValue("");
  }

  void testIsValid() {
    TS_ASSERT_EQUALS(wsp1->isValid(), "Workspace \"ws1\" was not found in the Analysis Data Service");
    TS_ASSERT_EQUALS(wsp2->isValid(), "Enter a name for the Output workspace");
    TS_ASSERT_EQUALS(wsp3->isValid(), "Workspace \"ws3\" was not found in the Analysis Data Service");
    TS_ASSERT_EQUALS(wsp4->isValid(), "");
    TS_ASSERT_EQUALS(wsp6->isValid(), "Enter a name for the Output workspace");

    // Setting a valid workspace name should make wsp2 (an output workspace)
    // valid
    TS_ASSERT_EQUALS(wsp2->setValue("ws2"), "");
    TS_ASSERT_EQUALS(wsp2->isValid(), "");
    // Setting an invalid name should make wsp6 invalid
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    AnalysisDataService::Instance().setIllegalCharacterList(illegalChars);
    std::string error = "Invalid object name 'ws6-1'. Names cannot contain any "
                        "of the following characters: " +
                        illegalChars;
    TS_ASSERT_EQUALS(wsp6->setValue("ws6-1"), error);
    TS_ASSERT_EQUALS(wsp6->isValid(), error);
    AnalysisDataService::Instance().setIllegalCharacterList("");

    // The other three need the input workspace to exist in the ADS
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1));
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add("ws1", space));
    wsp1->setValue("ws1");
    TS_ASSERT_EQUALS(wsp1->isValid(), "");

    // Put workspace of wrong type and check validation fails
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add("ws3", space));
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS(wsp3->isValid(), "Workspace ws3 is not of the correct type");
    // Now put correct type in and check it passes
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest2", 1, 1, 1))
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace("ws3", space));
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS(wsp3->isValid(), "");

    // The optional one
    wsp4->setValue("ws1");
    TS_ASSERT_EQUALS(wsp4->isValid(), "");
    // Check incorrect type
    wsp5->setValue("ws1");
    TS_ASSERT_EQUALS(wsp5->isValid(), "Workspace ws1 is not of the correct type");
    // Now the correct type
    wsp5->setValue("ws3");
    TS_ASSERT_EQUALS(wsp5->isValid(), "");
  }

  void testIsDefaultAndGetDefault() {
    // The constructor set wsp2 = "" so getDefault should always equal "", we'll
    // change the value and check
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
    // change the value to something else anything
    wsp2->setValue("ws2");
    // it is not default now
    TS_ASSERT(!wsp2->isDefault())
    // is default should stay the same
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
    wsp2->setValue("");
    TS_ASSERT(wsp2->isDefault())
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
  }

  void testIsDefaultWorksOnUnnamedWorkspaces() {
    std::string defaultWSName{""};
    WorkspaceProperty<Workspace> p("PropertyName", defaultWSName, Direction::InOut);
    TS_ASSERT(p.isDefault())
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    p.setDataItem(ws);
    TS_ASSERT(!p.isDefault())
    TS_ASSERT_EQUALS(p.value(), defaultWSName)
    defaultWSName = "default";
    WorkspaceProperty<Workspace> p2("PropertyName", defaultWSName, Direction::Input);
    TS_ASSERT(p2.isDefault())
    p2.setDataItem(ws);
    TS_ASSERT(!p2.isDefault())
    TS_ASSERT_EQUALS(p2.value(), "")
  }

  void testAllowedValues() {
    std::vector<std::string> vals;
    TS_ASSERT_THROWS_NOTHING(vals = wsp1->allowedValues())
    TS_ASSERT_EQUALS(vals.size(), 2)
    TS_ASSERT(std::find(vals.begin(), vals.end(), "ws1") != vals.end())
    TS_ASSERT(std::find(vals.begin(), vals.end(), "ws3") != vals.end())

    TS_ASSERT(wsp2->allowedValues().empty())

    TS_ASSERT_THROWS_NOTHING(vals = wsp3->allowedValues())
    TS_ASSERT_EQUALS(vals.size(), 1)
  }

  void testInvalidAllowedValues() {
    std::vector<std::string> vals;
    WorkspaceProperty<TableWorkspaceTester> testTblProperty("Table Mismatch test", "ws3", Direction::Input);
    WorkspaceProperty<WorkspaceGroup> testGroupProperty("Group Mismatch test", "ws1", Direction::Input);

    TS_ASSERT_THROWS_NOTHING(vals = testTblProperty.allowedValues());
    TS_ASSERT_EQUALS(vals.size(), 0);

    TS_ASSERT_THROWS_NOTHING(vals = testGroupProperty.allowedValues());
    TS_ASSERT_EQUALS(vals.size(), 0);
  }

  void testCreateHistory() {
    PropertyHistory history = wsp1->createHistory();
    TS_ASSERT_EQUALS(history.name(), "workspace1")
    TS_ASSERT_EQUALS(history.value(), "ws1")
    TS_ASSERT(history.isDefault())
    TS_ASSERT_EQUALS(history.type(), wsp1->type())
    TS_ASSERT_EQUALS(history.direction(), 0)

    // change the name back to ws2 to check that isDefault() fails
    wsp2->setValue("ws2");
    PropertyHistory history2 = wsp2->createHistory();
    TS_ASSERT_EQUALS(history2.name(), "workspace2")
    TS_ASSERT_EQUALS(history2.value(), "ws2")
    TS_ASSERT(!history2.isDefault())
    TS_ASSERT_EQUALS(history2.type(), wsp2->type())
    TS_ASSERT_EQUALS(history2.direction(), 1)

    // create empty workspace with blank name
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1));
    auto wsp7 = new WorkspaceProperty<Workspace>("workspace7", "", Direction::Input);
    *wsp7 = space;
    TS_ASSERT(wsp7->getWorkspace())

    // test the history contains an empty name
    PropertyHistory history3 = wsp7->createHistory();
    TS_ASSERT_EQUALS(history3.name(), "workspace7")
    TS_ASSERT(!history3.value().empty())
    TS_ASSERT_EQUALS(history3.value().substr(0, 5), "__TMP")
    TS_ASSERT_EQUALS(history3.type(), wsp7->type())
    TS_ASSERT_EQUALS(history3.direction(), 0)
    wsp7->setValue("ws2");
    delete wsp7;
  }

  void testStore() {
    // This is an input workspace so should return false
    TS_ASSERT(!wsp1->store())

    // Since no workspace has been assigned to this output property, it should
    // throw
    TS_ASSERT_THROWS(wsp2->store(), const std::runtime_error &)
    // So now create and assign the workspace and test again
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1));
    *wsp2 = space;
    TS_ASSERT(wsp2->store())
    // Check it really has been stored in the ADS
    Workspace_sptr storedspace;
    TS_ASSERT_THROWS_NOTHING(storedspace = AnalysisDataService::Instance().retrieve("ws2"))
    TS_ASSERT_EQUALS(storedspace->id(), "WorkspacePropTest")

    // This one should pass
    TS_ASSERT(wsp3->store())

    // Should be cleared as part of store so these should be empty
    TS_ASSERT(!wsp1->operator()())
    TS_ASSERT(!wsp2->operator()())
    TS_ASSERT(!wsp3->operator()())
  }

  void testTempName() {
    wsp4->setValue("");
    // Create and assign the workspace
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1));
    *wsp4 = space;

    PropertyHistory history = wsp4->createHistory();
    TS_ASSERT(!history.value().empty())
    TS_ASSERT_EQUALS(history.value().substr(0, 5), "__TMP")
  }

  void testDirection() {
    TS_ASSERT_EQUALS(wsp1->direction(), 0);
    TS_ASSERT_EQUALS(wsp2->direction(), 1);
    TS_ASSERT_EQUALS(wsp3->direction(), 2);
    TS_ASSERT_EQUALS(wsp4->direction(), 0);
    TS_ASSERT_EQUALS(wsp5->direction(), 0);
  }

  void test_locking() {
    // All the default ones are locking.
    TS_ASSERT(wsp1->isLocking());
    TS_ASSERT(wsp2->isLocking());
    TS_ASSERT(wsp3->isLocking());
    TS_ASSERT(wsp4->isLocking());
    TS_ASSERT(wsp5->isLocking());

    // Create one that is not locking
    WorkspaceProperty<Workspace> p1("workspace1", "ws1", Direction::Input, PropertyMode::Mandatory, LockMode::NoLock);
    TS_ASSERT(!p1.isLocking());

    // Copy constructor, both ways
    WorkspaceProperty<Workspace> wsp1_copy(*wsp1);
    TS_ASSERT(wsp1_copy.isLocking());
    WorkspaceProperty<Workspace> p2(p1);
    TS_ASSERT(!p2.isLocking());
  }

  void test_storing_workspace_name_assing() {
    Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    AnalysisDataService::Instance().add("space1", ws1);
    WorkspaceProperty<Workspace> p1("workspace1", "", Direction::Input);
    p1 = ws1;
    TS_ASSERT_EQUALS(p1.value(), "space1");
    AnalysisDataService::Instance().clear();
  }

  void test_storing_workspace_name_setDataItem() {
    Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    AnalysisDataService::Instance().add("space1", ws1);
    WorkspaceProperty<Workspace> p1("workspace1", "", Direction::Input);
    p1.setDataItem(ws1);
    TS_ASSERT_EQUALS(p1.value(), "space1");
    AnalysisDataService::Instance().clear();
  }

  void test_not_storing_workspace_name() {
    Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    WorkspaceProperty<Workspace> p1("workspace1", "", Direction::Input);
    p1 = ws1;
    TS_ASSERT_EQUALS(p1.value(), "");
  }

  void test_trimmming() {
    // trimming on
    Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    AnalysisDataService::Instance().add("space1", ws1);
    WorkspaceProperty<Workspace> p1("workspace1", "", Direction::Input);
    p1.setValue("  space1\t\n");
    TS_ASSERT_EQUALS(p1.value(), "space1");

    // turn trimming off
    Workspace_sptr ws2 = WorkspaceFactory::Instance().create("WorkspacePropertyTest", 1, 1, 1);
    AnalysisDataService::Instance().add("  space1\t\n", ws2);
    WorkspaceProperty<Workspace> p2("workspace1", "", Direction::Input);
    p2.setAutoTrim(false);
    p2.setValue("  space1\t\n");
    TS_ASSERT_EQUALS(p2.value(), "  space1\t\n");

    AnalysisDataService::Instance().clear();
  }

private:
  WorkspacePropertyWorkspace_uptr wsp1;
  WorkspacePropertyWorkspace_uptr wsp2;
  WorkspacePropertyWorkspaceTester2_uptr wsp3;
  WorkspacePropertyWorkspace_uptr wsp4;
  WorkspacePropertyWorkspaceTester2_uptr wsp5;
  WorkspacePropertyWorkspace_uptr wsp6;
};
