// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_MultiPeriodGroupAlgorithmTEST_H_
#define MANTID_API_MultiPeriodGroupAlgorithmTEST_H_

#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MultiPeriodGroupTestBase.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

// ------------------------------------------------------------------
// Working, concrete MultiPeriodGroupAlgorithm. With single input array
// property.
class TestAlgorithmA : public MultiPeriodGroupAlgorithm {
public:
  TestAlgorithmA() {}
  const std::string name() const override { return "TestAlgorithmA"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty(
        make_unique<ArrayProperty<std::string>>("MyInputWorkspaces"));
    declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                     Direction::Output),
                    "");
    declareProperty(
        "PropertyA", 1,
        boost::make_shared<Kernel::MandatoryValidator<int>>()); // I'm only
                                                                // adding this
                                                                // property to
                                                                // cause errors
                                                                // if it's not
                                                                // passed to
                                                                // spawned
                                                                // algorithms.
  }
  void exec() override {
    setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
  }
  std::string fetchInputPropertyName() const override {
    return "MyInputWorkspaces";
  }
  bool useCustomInputPropertyName() const override { return true; }
  ~TestAlgorithmA() override {}
};
DECLARE_ALGORITHM(TestAlgorithmA)
// ------------------------------------------------------------------
// End class dec

// ------------------------------------------------------------------
// Working, concrete MultiPeriodGroupAlgorithm. With proper named group input
// properties.
class TestAlgorithmB : public MultiPeriodGroupAlgorithm {
public:
  TestAlgorithmB() {}
  const std::string name() const override { return "TestAlgorithmB"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty(
        make_unique<WorkspaceProperty<>>("PropertyA", "ws1", Direction::Input));
    declareProperty(
        make_unique<WorkspaceProperty<>>("PropertyB", "ws2", Direction::Input));
    declareProperty(
        make_unique<WorkspaceProperty<>>("PropertyC", "ws3", Direction::Input));
    declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                     Direction::Output),
                    "");
    declareProperty(
        "PropertyX", 1,
        boost::make_shared<Kernel::MandatoryValidator<int>>()); // I'm only
                                                                // adding this
                                                                // property to
                                                                // cause errors
                                                                // if it's not
                                                                // passed to
                                                                // spawned
                                                                // algorithms.
  }
  void exec() override {
    MatrixWorkspace_sptr a = getProperty("PropertyA");
    MatrixWorkspace_sptr b = getProperty("PropertyB");
    MatrixWorkspace_sptr c = getProperty("PropertyC");
    int x = getProperty("PropertyX");
    UNUSED_ARG(a);
    UNUSED_ARG(b);
    UNUSED_ARG(c);
    UNUSED_ARG(x);

    setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
  }
  std::string fetchInputPropertyName() const override { return ""; }
  bool useCustomInputPropertyName() const override { return false; }
  ~TestAlgorithmB() override {}
};
DECLARE_ALGORITHM(TestAlgorithmB)
// ------------------------------------------------------------------
// End class dec

class MultiPeriodGroupAlgorithmTest : public CxxTest::TestSuite,
                                      public MultiPeriodGroupTestBase {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupAlgorithmTest *createSuite() {
    return new MultiPeriodGroupAlgorithmTest();
  }
  static void destroySuite(MultiPeriodGroupAlgorithmTest *suite) {
    delete suite;
  }

  // Note that we may wish to retire this test if we support other input
  // property types in the future.
  void test_input_property_not_string_array_throws() {
    // ------------------------------------------------------------------
    // Test Algorithm with input property that is not an array.
    class BrokenAlgorithm : public MultiPeriodGroupAlgorithm {
    public:
      const std::string name() const override { return "BrokenAlgorithm"; }
      int version() const override { return 1; }
      const std::string summary() const override { return "Test summary"; }
      void init() override {
        declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                            "InputWorkspaces", "", Direction::Input),
                        "");
        declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                         Direction::Output),
                        "");
      }
      void exec() override {
        setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
      }
      std::string fetchInputPropertyName() const override {
        return "InputWorkspaces";
      }
      bool useCustomInputPropertyName() const override { return true; }
      ~BrokenAlgorithm() override {}
    };
    // ------------------------------------------------------------------
    // End class dec

    WorkspaceGroup_sptr testInput =
        create_good_multiperiod_workspace_group("test");

    BrokenAlgorithm alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", testInput);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TSM_ASSERT_THROWS("Should throw because fetchInputPropertyName is "
                      "returning the name of a property which doesn't exist.",
                      alg.execute(), const std::runtime_error &);
  }

  void test_input_property_doesnt_exist_throws() {
    // ------------------------------------------------------------------
    // Test Algorithm with fetchInputPropertyName incorrectly wired-up.
    class BrokenAlgorithm : public MultiPeriodGroupAlgorithm {
    public:
      const std::string name() const override { return "BrokenAlgorithm"; }
      int version() const override { return 1; }
      const std::string summary() const override { return "Test summary"; }
      void init() override {
        declareProperty(
            make_unique<ArrayProperty<std::string>>("InputWorkspaces"));
        declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                         Direction::Output),
                        "Name of the output workspace");
      }
      void exec() override {
        setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
      }
      std::string fetchInputPropertyName() const override {
        return "made_up_property_name";
      }
      bool useCustomInputPropertyName() const override { return true; }
      ~BrokenAlgorithm() override {}
    };
    // ------------------------------------------------------------------
    // End class dec

    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");

    BrokenAlgorithm alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", "a, b");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TSM_ASSERT_THROWS("Should throw because fetchInputPropertyName is "
                      "returning the name of a property which doesn't exist.",
                      alg.execute(), const Kernel::Exception::NotFoundError &);
  }

  void test_process_groups_with_array_input() {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");
    WorkspaceGroup_sptr c = create_good_multiperiod_workspace_group("c");

    TestAlgorithmA alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("MyInputWorkspaces", "a, b, c");
    alg.setProperty("PropertyA", 1);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr wsgroup =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "outWS");
    TS_ASSERT(wsgroup != nullptr);
    TS_ASSERT_EQUALS(a->size(), wsgroup->size());
  }

  void xtest_process_groups_with_workspace_type_inputs() {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");
    WorkspaceGroup_sptr c = create_good_multiperiod_workspace_group("c");

    AnalysisDataService::Instance().addOrReplace("ws1", a);
    AnalysisDataService::Instance().addOrReplace("ws2", b);
    AnalysisDataService::Instance().addOrReplace("ws3", c);

    TestAlgorithmB alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PropertyA", a);
    alg.setProperty("PropertyB", b);
    alg.setProperty("PropertyC", c);
    alg.setProperty("PropertyX", 1);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr wsgroup =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "outWS");
    TS_ASSERT(wsgroup != nullptr);
    TS_ASSERT_EQUALS(a->size(), wsgroup->size());
  }
};

#endif /* MANTID_API_MultiPeriodGroupAlgorithmTEST_H_ */
