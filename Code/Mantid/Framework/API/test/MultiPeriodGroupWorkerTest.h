#ifndef MANTID_API_MULTIPERIODGROUPWORKERTEST_H_
#define MANTID_API_MULTIPERIODGROUPWORKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MultiPeriodGroupWorker.h"
#include "MultiPeriodGroupTestBase.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::MultiPeriodGroupWorker;
using namespace Mantid::API;

namespace
{
  class TestAlgorithm: public Algorithm
  {
  public:
    TestAlgorithm()
    {
    }
    virtual const std::string name() const
    {
      return "TestAlgorithm";
    }
    virtual int version() const
    {
      return 1;
    }
    virtual const std::string summary() const
    {
      return "Test summary";
    }
    virtual void init()
    {
      declareProperty(new ArrayProperty<std::string>("MyInputWorkspaces", Direction::Input));
      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output), "");
    }
    virtual void exec()
    {
      setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
    }
    virtual ~TestAlgorithm()
    {
    }
  };
  DECLARE_ALGORITHM(TestAlgorithm)
}

class MultiPeriodGroupWorkerTest: public CxxTest::TestSuite, public MultiPeriodGroupTestBase
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupWorkerTest *createSuite()
  {
    return new MultiPeriodGroupWorkerTest();
  }
  static void destroySuite(MultiPeriodGroupWorkerTest *suite)
  {
    delete suite;
  }

  void test_default_construction()
  {
    MultiPeriodGroupWorker worker;
    TS_ASSERT(!worker.useCustomWorkspaceProperty());
  }

  void test_regular_construction()
  {
    MultiPeriodGroupWorker worker1("InputWorkspace");
    TS_ASSERT(worker1.useCustomWorkspaceProperty());

    MultiPeriodGroupWorker worker2("InputWorkspace");
    TS_ASSERT(worker2.useCustomWorkspaceProperty());
  }

  void test_findGroups()
  {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");

    MultiPeriodGroupWorker worker("MyInputWorkspaces");

    auto alg = boost::make_shared<TestAlgorithm>();
    alg->initialize();
    alg->setPropertyValue("MyInputWorkspaces", "a, b");

    auto groups = worker.findMultiPeriodGroups(alg.get());

    TS_ASSERT_EQUALS(groups.size(), 2);
  }

  void test_processGroups()
  {
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
    auto out_group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out_ws");
    TS_ASSERT_EQUALS(a->size(), out_group->size());
    AnalysisDataService::Instance().remove("out_ws");
  }

};

#endif /* MANTID_API_MULTIPERIODGROUPWORKERTEST_H_ */
