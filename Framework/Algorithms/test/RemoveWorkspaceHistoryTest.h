// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REMOVEWORKSPACEHISTORYTEST_H_
#define MANTID_ALGORITHMS_REMOVEWORKSPACEHISTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/RemoveWorkspaceHistory.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::Kernel;
using Mantid::Algorithms::RemoveWorkspaceHistory;

class RemoveWorkspaceHistoryTest : public CxxTest::TestSuite {
private:
  /// Use a fake algorithm object instead of a dependency on a real one.
  class SimpleSum : public Algorithm {
  public:
    SimpleSum() : Algorithm() {}
    ~SimpleSum() override {}
    const std::string name() const override { return "SimpleSum"; }
    const std::string summary() const override { return "SimpleSum"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }

    void init() override {
      declareProperty(
          std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut),
          "");
      declareProperty("Input1", 2);
      declareProperty("Input2", 1);
      declareProperty("Output1", -1, Direction::Output);
    }
    void exec() override {
      const int lhs = getProperty("Input1");
      const int rhs = getProperty("Input2");
      const int sum = lhs + rhs;

      setProperty("Output1", sum);
    }
  };

  class SimpleSum2 : public SimpleSum {
  public:
    const std::string name() const override { return "SimpleSum2"; }
    const std::string summary() const override { return "SimpleSum2"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }

    void init() override {
      SimpleSum::init();
      declareProperty("Input3", 4);
      declareProperty("Output2", -1, Direction::Output);
    }
    void exec() override {
      SimpleSum::exec();
      int sum = this->getProperty("Output1");
      setProperty("Output2", sum + 1);
    }
  };

  void createWorkspace(std::string wsName) {
    // create a fake workspace for testing
    boost::shared_ptr<WorkspaceTester> input =
        boost::make_shared<WorkspaceTester>();
    AnalysisDataService::Instance().addOrReplace(wsName, input);

    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum2>();

    // run some dummy algorithms
    SimpleSum simplesum;
    simplesum.initialize();
    simplesum.setPropertyValue("Workspace", wsName);
    simplesum.setPropertyValue("Input1", "5");
    simplesum.execute();

    SimpleSum2 simplesum2;
    simplesum2.initialize();
    simplesum2.setPropertyValue("Workspace", wsName);
    simplesum2.setPropertyValue("Input3", "10");
    simplesum2.execute();

    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum2", 1);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveWorkspaceHistoryTest *createSuite() {
    return new RemoveWorkspaceHistoryTest();
  }
  static void destroySuite(RemoveWorkspaceHistoryTest *suite) { delete suite; }

  void test_Init() {
    RemoveWorkspaceHistory alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    std::string wsName = "__remove_history_test_workspace";
    createWorkspace(wsName);

    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName));
    TS_ASSERT(!ws->history().empty());
    TS_ASSERT_EQUALS(ws->history().size(), 2);

    RemoveWorkspaceHistory alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    TS_ASSERT_EQUALS(ws->history().size(), 1);
    TS_ASSERT_EQUALS(ws->history().getAlgorithmHistory(0)->name(),
                     "RemoveWorkspaceHistory");

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_REMOVEWORKSPACEHISTORYTEST_H_ */
