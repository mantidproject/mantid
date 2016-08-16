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
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceListProperty.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <gmock/gmock.h>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;

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
  template <typename T> class MyAlgorithm : public Mantid::API::Algorithm {
  public:
    MyAlgorithm() { this->setRethrows(true); }

    virtual int version() const override { return 1; }

    virtual const std::string name() const override { return "MyAlgorithm"; }

    const std::string summary() const override { return "MyAlgorithm helper."; }

    virtual void init() override {
      declareProperty(Kernel::make_unique<WorkspaceListProperty<T>>(
          "MyProperty", std::vector<boost::shared_ptr<T>>(0)));
    }

    virtual void exec() override {
      std::vector<boost::shared_ptr<T>> val = getProperty("MyProperty");
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

    TS_ASSERT_THROWS_NOTHING(
        Kernel::make_unique<WorkspaceListProperty<Workspace>>(
            WorkspaceListProperty<Workspace>("MyWorkspaceProperty", list,
                                             Direction::Input,
                                             PropertyMode::Mandatory)));
  }

  void test_contruct_as_optional() {
    auto list = std::vector<Workspace_sptr>();

    TS_ASSERT_THROWS_NOTHING(
        Kernel::make_unique<WorkspaceListProperty<Workspace>>(
            "MyWorkspaceProperty", list, Direction::Input,
            PropertyMode::Optional));
  }

  void test_construct_single_workspace() {
    auto wksp = boost::make_shared<WorkspaceTester>();
    WorkspaceListProperty<Workspace> prop(
        "MyWorkspaceProperty", wksp, Direction::Input, PropertyMode::Mandatory);

    auto list = prop.list();

    TS_ASSERT_EQUALS(wksp, list[0]);
  }

  void test_copy_construction() {
    auto list = createWorkspaceList();

    WorkspaceListProperty<Workspace> a("PropA", list, Direction::Input,
                                       PropertyMode::Optional);
    WorkspaceListProperty<Workspace> b(a);

    TS_ASSERT_EQUALS(list, a.list());
    TS_ASSERT_EQUALS(a.list(), b.list());
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
    TS_ASSERT_EQUALS(list, propA.list());
    TS_ASSERT_EQUALS(propA.isOptional(), propB.isOptional());
    TS_ASSERT_EQUALS(propA.list(), propB.list());
  }

  void test_clone() {
    auto list = createWorkspaceList();
    WorkspaceListProperty<Workspace> propOne("PropA", list, Direction::Input,
                                             PropertyMode::Optional);
    WorkspaceListProperty<Workspace> propTwo = *propOne.clone();
    TS_ASSERT_EQUALS(propOne, propTwo);
  }

  //------------------------------------------------------------------------------
  // Integration type testing. Test that the Property works nicely via the
  // PropertyManager interfaces (such as Algorithm).
  //------------------------------------------------------------------------------
  void test_set_and_get_Property() {
    using Alg = MyAlgorithm<Workspace>;
    Alg alg;
    alg.initialize();

    auto ilist = createWorkspaceList();
    alg.setProperty("MyProperty", ilist);
    // Now fetch the property.
    std::vector<Workspace_sptr> olist = alg.getProperty("MyProperty");

    validateWorkspaceList(ilist, olist);
  }

  void test_multiple_workspace_types() {
    auto a = boost::make_shared<WorkspaceTester>();
    auto b = boost::make_shared<TableWorkspaceTester>();

    auto ilist = std::vector<Workspace_sptr>();

    ilist.push_back(a);
    ilist.push_back(b);

    using Alg = MyAlgorithm<Workspace>;
    Alg alg;
    ;
    alg.initialize();

    alg.setProperty("MyProperty", ilist);

    std::vector<Workspace_sptr> olist = alg.getProperty("MyProperty");

    TS_ASSERT_EQUALS(olist.size(), ilist.size());

    auto oa = boost::dynamic_pointer_cast<WorkspaceTester>(olist[0]);
    auto ob = boost::dynamic_pointer_cast<TableWorkspaceTester>(olist[1]);

    std::vector<Workspace_sptr> tmpList{oa, ob};

    TS_ASSERT_EQUALS(ilist, tmpList);
  }

  void test_multiple_types_with_specific_template() {
    auto wksp = boost::make_shared<WorkspaceTester>();
    auto tableWksp = boost::make_shared<TableWorkspaceTester>();
    auto group = boost::make_shared<WorkspaceGroup>();

    group->addWorkspace(boost::make_shared<WorkspaceTester>());
    group->addWorkspace(boost::make_shared<WorkspaceTester>());

    std::vector<Workspace_sptr> list{wksp, group};

    using Alg = MyAlgorithm<MatrixWorkspace>;
    Alg alg; // Property template specified as MatrixWorkspace
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("MyProperty", list),
                     std::invalid_argument);

    list.pop_back();
    list.push_back(tableWksp);

    TS_ASSERT_THROWS(alg.setProperty("MyProperty", list),
                     std::invalid_argument);
  }

  void test_invalid_list_type_fail() {
    std::vector<double> list;

    list.push_back(10);

    using Alg = MyAlgorithm<MatrixWorkspace>;
    Alg alg; // Property template specified as MatrixWorkspace
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("MyProperty", list),
                     std::invalid_argument);
  }

  void test_set_property_single_workspace() {
    auto wksp = boost::make_shared<WorkspaceTester>();
    using Alg = MyAlgorithm<Workspace>;
    Alg alg;

    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MyProperty", wksp));

    std::vector<Workspace_sptr> list = alg.getProperty("MyProperty");

    TS_ASSERT_EQUALS(list[0], wksp);
  }

  void test_set_property_workspace_groups() {
    auto group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(boost::make_shared<WorkspaceTester>());
    group->addWorkspace(boost::make_shared<WorkspaceTester>());

    using Alg = MyAlgorithm<WorkspaceGroup>;
    Alg alg;
    alg.initialize();

    auto list = std::vector<WorkspaceGroup_sptr>();

    list.push_back(group);

    alg.setProperty("MyProperty", list);

    std::vector<WorkspaceGroup_sptr> olist = alg.getProperty("MyProperty");

    auto ogroup = olist[0];

    TS_ASSERT_EQUALS(group->size(), ogroup->size());

    for (size_t i = 0; i < group->size(); i++)
      TS_ASSERT_EQUALS(group->getItem(i), ogroup->getItem(i));
  }

  void test_workspace_groups_in_ads_fail() {
    auto a = boost::make_shared<WorkspaceTester>();
    auto b = boost::make_shared<WorkspaceTester>();

    AnalysisDataService::Instance().add("a", a);

    auto group = boost::make_shared<WorkspaceGroup>();

    group->addWorkspace(a);
    group->addWorkspace(b);
    std::vector<WorkspaceGroup_sptr> list{group};

    TS_ASSERT_THROWS(
        WorkspaceListProperty<WorkspaceGroup>("Prop", list, Direction::Input),
        std::invalid_argument);
  }

  // TODO: Not sure what is the right thing to return here. One possibility
  // could be to serialise the contents of the workspace but this could be
  // an expensive operation.
  // Alternatively, and also quite expensive, could be to hash the contents of
  // the workspace.
  void test_return_property_as_string() {
    auto ilist = createWorkspaceList();
    using Alg = MyAlgorithm<Workspace>;
    Alg alg;
    alg.initialize();

    alg.setProperty("MyProperty", ilist);

    std::string strlist = alg.getProperty("MyProperty");

    TS_ASSERT_EQUALS(strlist, "");
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

  void validateWorkspaceList(const std::vector<Workspace_sptr> &wkspList,
                             const std::vector<Workspace_sptr> &diList) {
    TS_ASSERT_EQUALS(wkspList.size(), diList.size());

    TS_ASSERT_EQUALS(wkspList, diList);
  }
};
#endif /* MANTID_API_WORKSPACELISTPROPERTYTEST_H_ */