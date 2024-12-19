// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class DataProcessorAlgorithmTest : public CxxTest::TestSuite {

  // top level algorithm which executes -> NestedAlgorithm which executes ->
  // BasicAlgorithm
  class SubAlgorithm : public Algorithm {
  public:
    SubAlgorithm() : Algorithm() {}
    ~SubAlgorithm() override = default;
    const std::string name() const override { return "SubAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "SubAlgorithm"; }
    const std::string workspaceMethodName() const override { return "methodname"; }
    const std::string workspaceMethodOnTypes() const override { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

    void init() override {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() override {
      // nothing to do!
    }
  };

  // basic algorithm. This acts as a child called for other
  // DataProcessorAlgorithms
  class BasicAlgorithm : public Algorithm {
  public:
    BasicAlgorithm() : Algorithm() {}
    ~BasicAlgorithm() override = default;
    const std::string name() const override { return "BasicAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "BasicAlgorithm"; }
    const std::string workspaceMethodName() const override { return "methodname"; }
    const std::string workspaceMethodOnTypes() const override { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

    void init() override {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() override {
      // the history from this should never be stored
      auto alg = createChildAlgorithm("SubAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "I Don't exist!");
      alg->execute();
    }
  };

  // middle layer algorithm executed by a top level algorithm
  class NestedAlgorithm : public DataProcessorAlgorithm {
  public:
    NestedAlgorithm() : DataProcessorAlgorithm() {}
    ~NestedAlgorithm() override = default;
    const std::string name() const override { return "NestedAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "NestedAlgorithm"; }
    const std::string workspaceMethodName() const override { return "methodname"; }
    const std::string workspaceMethodOnTypes() const override { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

    void init() override {
      declareProperty("PropertyA", 13);
      declareProperty("PropertyB", 42);
    }

    void exec() override {
      auto alg = createChildAlgorithm("BasicAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "Same!");
      alg->execute();
    }
  };

  class TopLevelAlgorithm : public DataProcessorAlgorithm {
  public:
    TopLevelAlgorithm() : DataProcessorAlgorithm() {}
    ~TopLevelAlgorithm() override = default;
    const std::string name() const override { return "TopLevelAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "TopLevelAlgorithm"; }
    const std::string workspaceMethodName() const override { return "methodname"; }
    const std::string workspaceMethodOnTypes() const override { return "Workspace;MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

    void init() override {
      declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input));
      declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output));
      declareProperty("RecordHistory", true, Direction::Input);
    }
    void exec() override {
      const bool recordHistory = static_cast<bool>(getProperty("RecordHistory"));
      auto alg = createChildAlgorithm("NestedAlgorithm");
      alg->enableHistoryRecordingForChild(recordHistory);
      alg->initialize();
      alg->execute();

      std::shared_ptr<MatrixWorkspace> output = std::make_shared<WorkspaceTester>();
      setProperty("OutputWorkspace", output);
    }
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmTest *createSuite() { return new DataProcessorAlgorithmTest(); }
  static void destroySuite(DataProcessorAlgorithmTest *suite) { delete suite; }

  void setUp() override {
    Mantid::API::AlgorithmFactory::Instance().subscribe<TopLevelAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<NestedAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<BasicAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SubAlgorithm>();
  }

  void tearDown() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("TopLevelAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("NestedAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("BasicAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SubAlgorithm", 1);
  }

  void test_Nested_History() {
    std::shared_ptr<WorkspaceTester> input = std::make_shared<WorkspaceTester>();
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    TopLevelAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "test_output_workspace");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // check workspace history
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    TS_ASSERT_EQUALS(wsHist.size(), 1);

    // check top level algorithm history
    auto algHist = wsHist.getAlgorithmHistory(0);

    TS_ASSERT_EQUALS(algHist->name(), "TopLevelAlgorithm");
    TS_ASSERT_EQUALS(algHist->childHistorySize(), 1);

    // check nested algorithm history
    auto childHist = algHist->getChildAlgorithmHistory(0);

    TS_ASSERT_EQUALS(childHist->name(), "NestedAlgorithm");
    TS_ASSERT_EQUALS(childHist->childHistorySize(), 1);

    // check basic algorithm history
    childHist = childHist->getChildAlgorithmHistory(0);
    TS_ASSERT_EQUALS(childHist->name(), "BasicAlgorithm");

    // even though BasicAlgorithm calls another algorithm,
    // it should not store the history.
    TS_ASSERT_EQUALS(childHist->childHistorySize(), 0);

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Dont_Record_Nested_History() {
    std::shared_ptr<WorkspaceTester> input = std::make_shared<WorkspaceTester>();
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    TopLevelAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", input);
    alg.setProperty("RecordHistory", false);
    alg.setPropertyValue("OutputWorkspace", "test_output_workspace");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // check workspace history
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    TS_ASSERT_EQUALS(wsHist.size(), 1);

    auto algHist = wsHist.getAlgorithmHistory(0);
    TS_ASSERT_EQUALS(algHist->name(), "TopLevelAlgorithm");
    // algorithm should have no child histories.
    TS_ASSERT_EQUALS(algHist->childHistorySize(), 0);

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }
};
