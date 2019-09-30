// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_MULTIPERIODGROUPWORKERTEST_H_
#define MANTID_API_MULTIPERIODGROUPWORKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MultiPeriodGroupWorker.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MultiPeriodGroupTestBase.h"

using Mantid::API::MultiPeriodGroupWorker;
using namespace Mantid::API;

class TestAlgorithm : public Algorithm {
public:
  TestAlgorithm() {}
  const std::string name() const override { return "TestAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty(std::make_unique<ArrayProperty<std::string>>(
        "MyInputWorkspaces", Direction::Input));
    declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                          Direction::Output),
                    "");
  }
  void exec() override {
    setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
  }
  ~TestAlgorithm() override {}
};
DECLARE_ALGORITHM(TestAlgorithm)

class MultiPeriodGroupWorkerTest : public CxxTest::TestSuite,
                                   public MultiPeriodGroupTestBase {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupWorkerTest *createSuite() {
    return new MultiPeriodGroupWorkerTest();
  }
  static void destroySuite(MultiPeriodGroupWorkerTest *suite) { delete suite; }

  void test_default_construction() {
    MultiPeriodGroupWorker worker;
    TS_ASSERT(!worker.useCustomWorkspaceProperty());
  }

  void test_regular_construction() {
    MultiPeriodGroupWorker worker1("InputWorkspace");
    TS_ASSERT(worker1.useCustomWorkspaceProperty());

    MultiPeriodGroupWorker worker2("InputWorkspace");
    TS_ASSERT(worker2.useCustomWorkspaceProperty());
  }

  void test_findGroups() {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");

    MultiPeriodGroupWorker worker("MyInputWorkspaces");

    auto alg = boost::make_shared<TestAlgorithm>();
    alg->initialize();
    alg->setPropertyValue("MyInputWorkspaces", "a, b");

    auto groups = worker.findMultiPeriodGroups(alg.get());

    TS_ASSERT_EQUALS(groups.size(), 2);
  }

  void test_processGroups() {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");

    MultiPeriodGroupWorker worker("MyInputWorkspaces");

    auto alg = boost::make_shared<TestAlgorithm>();
    alg->initialize();
    alg->setPropertyValue("MyInputWorkspaces", "a");
    alg->setPropertyValue("OutputWorkspace", "out_ws");
    auto groups = worker.findMultiPeriodGroups(alg.get());

    TS_ASSERT_EQUALS(groups.size(), 1);

    TS_ASSERT(worker.processGroups(alg.get(), groups));

    AnalysisDataService::Instance().doesExist("out_ws");
    auto out_group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out_ws");
    TS_ASSERT_EQUALS(a->size(), out_group->size());
    AnalysisDataService::Instance().remove("out_ws");
  }
};

#endif /* MANTID_API_MULTIPERIODGROUPWORKERTEST_H_ */
