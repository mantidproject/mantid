#ifndef MANTID_API_WORKSPACELISTPROPERTYTEST_H_
#define MANTID_API_WORKSPACELISTPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceListProperty.h"
#include "MantidKernel/DataItem.h"
#include "MantidKernel/IValidator.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <gmock/gmock.h>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Kernel::DataItem;

class WorkspaceListPropertyTest : public CxxTest::TestSuite {
private:
  /**
  * Helper class used for Mocking validators.
  */
  class MockValidator : public Mantid::Kernel::IValidator {
  public:
    MockValidator() {}
    virtual ~MockValidator() {}
    MOCK_CONST_METHOD0(clone, IValidator_sptr());
    MOCK_CONST_METHOD1(check, std::string(const boost::any &));
  };

  /**
  * Helper class. Algorithms are instances of IPropertyManager
  */
  class MyAlgorithm : public Mantid::API::Algorithm {
  public:
    MyAlgorithm() { this->setRethrows(true); }

    virtual int version() const { return 1; }

    virtual const std::string name() const { return "MyAlgorithm"; }

    const std::string summary() const override { return "MyAlgorithm helper."; }

    virtual void init() {
      declareProperty(std::make_unique<WorkspaceListProperty<Workspace>>(
          "MyProperty", std::vector<Workspace_sptr>(0)));
    }

    virtual void exec() {
      std::vector<boost::shared_ptr<DataItem>> val = getProperty("MyProperty");
    }

    virtual ~MyAlgorithm() {}
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceListPropertyTest *createSuite() {
    return new WorkspaceListPropertyTest();
  }
  static void destroySuite(WorkspaceListPropertyTest *suite) { delete suite; }

  //------------------------------------------------------------------------------
  // Functional Testing
  //------------------------------------------------------------------------------

  void test_constructor() {
    auto list = createWorkspaceList();

    TS_ASSERT_THROWS_NOTHING(std::make_unique<WorkspaceListProperty<Workspace>>(
        WorkspaceListProperty<Workspace>("MyWorkspaceProperty", list,
                                         Direction::Input,
                                         PropertyMode::Mandatory)));
  }

  void test_contruct_as_optional() {
    auto list = std::vector<Workspace_sptr>();

    TS_ASSERT_THROWS_NOTHING(std::make_unique<WorkspaceListProperty<Workspace>>(
        WorkspaceListProperty<Workspace>("MyWorkspaceProperty", list,
                                         Direction::Input,
                                         PropertyMode::Optional)));
  }

  void test_copy_construction() {
    auto list = createWorkspaceList();

    WorkspaceListProperty<Workspace> a("PropA", list, Direction::Input,
                                       PropertyMode::Optional);
    WorkspaceListProperty<Workspace> b(a);

    TS_ASSERT_EQUALS(list, a.value());
    TS_ASSERT_EQUALS(a.value(), b.value());
    TS_ASSERT_EQUALS(a.isOptional(), b.isOptional());
  }

  void test_assignment() {
    auto list = createWorkspaceList();
    WorkspaceListProperty<Workspace> propA("PropA", list, Direction::Input,
                                           PropertyMode::Mandatory);
    WorkspaceListProperty<Workspace> propB(
        "PropB", std::vector<Workspace_sptr>(), Direction::Input,
        PropertyMode::Optional);
    propB = propA;
    TS_ASSERT_EQUALS(list, propA.value());
    TS_ASSERT_EQUALS(propA.value(), propB.value());
    TS_ASSERT_EQUALS(propA.isOptional(), propB.isOptional());
    AnalysisDataService::Instance().remove("a");
  }

  void test_custom_validator_usage() {
    using namespace testing;
    std::string okString = "";

    boost::shared_ptr<MockValidator> validator =
        boost::make_shared<MockValidator>();
    EXPECT_CALL(*validator.get(), check(_))
        .Times(2)
        .WillRepeatedly(Return(okString));

    auto ws = boost::make_shared<WorkspaceTester>();

    auto list = createWorkspaceList();
    WorkspaceListProperty<Workspace> prop("Prop", list, Direction::Input,
                                          PropertyMode::Mandatory, validator);

    TSM_ASSERT("Should repeatedly call the validator",
               Mock::VerifyAndClearExpectations(validator.get()));
  }

  void test_clone() {
    auto list = createWorkspaceList();
    WorkspaceListProperty<Workspace> *propOne =
        new WorkspaceListProperty<Workspace>("PropA", list, Direction::Input,
                                             PropertyMode::Optional);
    WorkspaceListProperty<Workspace> *propTwo = propOne->clone();
    TS_ASSERT_DIFFERS(propOne, propTwo);
    delete propOne;
    delete propTwo;
  }

  //------------------------------------------------------------------------------
  // Integration type testing. Test that the Property works nicely via the
  // PropertyManager interfaces (such as Algorithm).
  //------------------------------------------------------------------------------
  void test_set_and_get_Property() {
    MyAlgorithm alg;
    alg.initialize();

    auto ilist = createWorkspaceList();
    alg.setProperty("MyProperty", ilist);
    // Now fetch the property.
    std::vector<boost::shared_ptr<DataItem>> olist =
        alg.getProperty("MyProperty");

    validateDataItemList(ilist, olist);
  }

  void test_multiple_workspace_types() {
    auto a = boost::make_shared<WorkspaceTester>();
    auto b = boost::make_shared<TableWorkspaceTester>();

    auto ilist = std::vector<Workspace_sptr>();

    ilist.push_back(a);
    ilist.push_back(b);

    MyAlgorithm alg;
    alg.initialize();

    alg.setProperty("MyProperty", ilist);

    std::vector<boost::shared_ptr<DataItem>> olist =
        alg.getProperty("MyProperty");

    TS_ASSERT_EQUALS(olist.size(), ilist.size());

    auto oa = boost::dynamic_pointer_cast<WorkspaceTester>(olist[0]);
    auto ob = boost::dynamic_pointer_cast<TableWorkspaceTester>(olist[1]);

    std::vector<Workspace_sptr> tmpList{oa, ob};

    TS_ASSERT_EQUALS(ilist, tmpList);
  }

  void test_return_property_as_string() {
	  auto ilist = createWorkspaceList();
	  MyAlgorithm alg;
	  alg.initialize();

	  alg.setProperty("MyProperty", ilist);

	  std::string strlist = alg.getProperty("MyProperty");
  }

private:
  std::vector<Workspace_sptr> createWorkspaceList() const {
    auto a = boost::make_shared<WorkspaceTester>();
    auto b = boost::make_shared<WorkspaceTester>();
    auto list = std::vector<Workspace_sptr>();
    list.push_back(a);
    list.push_back(b);

    return list;
  }

  std::vector<Workspace_sptr>
  convertDataItemList(const std::vector<boost::shared_ptr<DataItem>> &list) {
    auto newList = std::vector<Workspace_sptr>();

    for (auto &i : list) {
      newList.push_back(boost::dynamic_pointer_cast<WorkspaceTester>(i));
    }

    return newList;
  }

  void
  validateDataItemList(const std::vector<Workspace_sptr> &wkspList,
                       const std::vector<boost::shared_ptr<DataItem>> &diList) {
    TS_ASSERT_EQUALS(wkspList.size(), diList.size());

    auto newList = convertDataItemList(diList);

    TS_ASSERT_EQUALS(wkspList, newList);
  }
};

#endif /* MANTID_API_WORKSPACELISTPROPERTYTEST_H_ */