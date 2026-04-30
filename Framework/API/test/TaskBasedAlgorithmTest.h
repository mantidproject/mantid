// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TaskBasedAlgorithm.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {
class ToyTaskBasedAlg : public Mantid::API::TaskBasedAlgorithm<ToyTaskBasedAlg> {
public:
  const std::string name() const override { return "ToyTaskBasedAlg"; };
  const std::string summary() const override { return "Toy alg for task based algorithm test"; }
  int version() const override { return 1; };
  void init() override {
    declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                        "InputWorkspace", "", Mantid::Kernel::Direction::Input),
                    "The input workspace to be passed to the first task");
    declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                        "OutputWorkspace", "", Mantid::Kernel::Direction::Output),
                    "Output workspace from final task");
    initTaskBasedAlgorithm<TaskA, TaskB, TaskC, TaskD, TaskDAlt, TaskE>();
  }
  void enableMutableInput() { setMutableInput(true); }
  void exec() override { execTasks(); };

  // Possible orders tested:
  // 1) A, B, C, D
  // 2) A, C, B, D
  // 3) A, B, D

  class TaskA final : public AlgorithmTask {
  public:
    explicit TaskA(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskA") { setExpectedOutputs({"TaskAOutput"}); }
    void executeImpl() override {
      auto inputWS = getDependantWorkspace("InputWorkspace");
      m_parent->scaleWorkspace(inputWS, 5.0);
      outputWorkspace(inputWS, "TaskAOutput");
      m_parent->outputDebugWorkspace(inputWS, "debug_test", "SUFF", 0);
    };
  };

  class TaskB final : public AlgorithmTask {
  public:
    explicit TaskB(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskB") {
      setExpectedOutputs({"TaskBOutput1", "TaskBOutput2"});
      setDependantTask("TaskA", "TaskAOutput", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskC", "TaskCOutput", "InputWorkspace", depSet);
    }
    void executeImpl() override {
      auto inputWS = getDependantWorkspace("InputWorkspace");
      auto cloneWS1 = m_parent->cloneWorkspace(inputWS);
      m_parent->translateWorkspace(cloneWS1, 2.5);
      auto cloneWS2 = m_parent->cloneWorkspace(inputWS);
      m_parent->translateWorkspace(cloneWS2, 1.25);
      outputWorkspace(cloneWS1, "TaskBOutput1");
      outputWorkspace(cloneWS2, "TaskBOutput2");
    };
  };

  class TaskC final : public AlgorithmTask {
  public:
    explicit TaskC(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskC") {
      setExpectedOutputs({"TaskCOutput"});
      setDependantTask("TaskB", "TaskBOutput1", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskA", "TaskAOutput", "InputWorkspace", depSet);
    }
    void executeImpl() override {
      auto inputWS = getDependantWorkspace("InputWorkspace");
      m_parent->scaleWorkspace(inputWS, 1.0 / 3.0);
      outputWorkspace(inputWS, "TaskCOutput");
    };
  };

  class TaskDBase : public AlgorithmTask {
  public:
    explicit TaskDBase(ToyTaskBasedAlg *parent, const std::string &name) : AlgorithmTask(parent, name) {
      setExpectedOutputs({"TaskDOutput1", "TaskDOutput2"});
      setDependantTask("TaskC", "TaskCOutput", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskB", "TaskBOutput2", "InputWorkspace", depSet);
    }
    void executeImpl() override {
      auto inputWS = getDependantWorkspace("InputWorkspace");
      auto cloneWS1 = m_parent->cloneWorkspace(inputWS);
      m_parent->translateWorkspace(cloneWS1, -2.50);
      auto cloneWS2 = m_parent->cloneWorkspace(inputWS);
      m_parent->translateWorkspace(cloneWS2, -3.75);
      outputWorkspace(cloneWS1, "TaskDOutput1");
      outputWorkspace(cloneWS2, "TaskDOutput2");
    }
  };

  class TaskD final : public TaskDBase {
  public:
    explicit TaskD(ToyTaskBasedAlg *parent) : TaskDBase(parent, "TaskD") { setSelectedOutput("TaskDOutput2"); }
  };

  // Task D with an alternate selected output.
  class TaskDAlt final : public TaskDBase {
  public:
    explicit TaskDAlt(ToyTaskBasedAlg *parent) : TaskDBase(parent, "TaskDAlt") { setSelectedOutput("TaskDOutput1"); }
  };

  class TaskE final : public AlgorithmTask {
  public:
    explicit TaskE(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskE") {
      setExpectedOutputs({"TaskEOutput"});
      setDependantTask("TaskD");
    }
    void executeImpl() override {
      auto inputWS = getDependantWorkspace("TaskDOutput2");
      m_parent->translateWorkspace(inputWS, 1.0);
      outputWorkspace(inputWS, "TaskEOutput");
    };
  };

  Mantid::API::MatrixWorkspace_sptr cloneWorkspace(const Mantid::API::MatrixWorkspace_sptr &ws) {
    auto cloneWorkspace = createChildAlgorithm("CloneWorkspace");
    cloneWorkspace->setProperty("InputWorkspace", ws);
    cloneWorkspace->execute();
    Mantid::API::Workspace_sptr wsClone = cloneWorkspace->getProperty("OutputWorkspace");
    return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsClone);
  }

  void scaleWorkspace(Mantid::API::MatrixWorkspace_sptr &ws, const double val) {
    auto &y = ws->mutableY(0);
    for (size_t i = 0; i < y.size(); ++i) {
      y[i] = y[i] * val;
    }
  }

  void translateWorkspace(Mantid::API::MatrixWorkspace_sptr &ws, const double val) {
    auto &y = ws->mutableY(0);
    for (size_t i = 0; i < y.size(); ++i) {
      y[i] = y[i] + val;
    }
  }
};

Mantid::API::MatrixWorkspace_sptr makeMatrixWorkspaceFromVector(const std::vector<double> &yValues,
                                                                const std::string &outputName = "tmp_ws") {
  const auto nBins = yValues.size();
  auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
  alg->initialize();
  alg->setProperty("WorkspaceType", "Histogram");
  alg->setProperty("NumBanks", 1);
  alg->setProperty("BankPixelWidth", 1);
  alg->setProperty("XMax", static_cast<double>(nBins));
  alg->setProperty("BinWidth", 1.0);
  alg->setProperty("OutputWorkspace", outputName);
  alg->setAlwaysStoreInADS(false);
  alg->execute();
  Mantid::API::MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");

  auto &x = ws->mutableX(0);
  auto &y = ws->mutableY(0);
  auto &e = ws->mutableE(0);
  for (size_t i = 0; i < x.size(); ++i) {
    x[i] = static_cast<double>(i);
  }
  std::copy(yValues.begin(), yValues.end(), y.begin());
  std::fill(e.begin(), e.end(), 0.0);
  return ws;
}

} // namespace

class TaskBasedAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TaskBasedAlgorithmTest *createSuite() { return new TaskBasedAlgorithmTest(); }
  static void destroySuite(TaskBasedAlgorithmTest *suite) { delete suite; }

  TaskBasedAlgorithmTest() {
    Mantid::API::FrameworkManager::Instance();
    Mantid::API::AnalysisDataService::Instance();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyTaskBasedAlg>();
  }

  ~TaskBasedAlgorithmTest() override { Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyTaskBasedAlg", 1); }

  void setUp() override { m_alg = Mantid::API::AlgorithmManager::Instance().create("ToyTaskBasedAlg"); }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void compareVectors(const std::vector<double> &vec1, const std::vector<double> &vec2) {
    if (vec1.size() != vec2.size()) {
      TSM_ASSERT("compareVectors: Vector sizes not equal", false);
      return;
    }

    for (size_t i = 0; i < vec1.size(); i++) {
      TS_ASSERT_DELTA(vec1[i], vec2[i], 0.001);
    }
  }

  void testTaskBasedAlgorithmABCD() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskC, TaskD");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {-1.25, 0.4167, 2.0833, 3.75, 5.4167});
  }

  void testTaskBasedAlgorithmACBD() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskC, TaskB, TaskD");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {-0.8333, 0.8333, 2.5, 4.1667, 5.8333});
  }

  void testTaskBasedAlgorithmABD() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskD");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {2.50, 7.50, 12.50, 17.50, 22.50});
  }

  void testTaskENoOutputSpecified() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskC, TaskD, TaskE");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {-0.25, 1.4167, 3.0833, 4.75, 6.4167});
  }

  void testTaskBasedAlgorithmThrowsWhenNoTaskSetCanBeFulfilled() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskD");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(
        m_alg->execute(), std::runtime_error & e, std::string(e.what()),
        "Cannot execute task TaskD as the following dependent tasks outputs are not available: Task set 0 "
        "unfulfillable as required tasks not staged: TaskC, Task set 1 unfulfillable as required tasks not staged: "
        "TaskB");
  }

  void testTaskBasedAlgorithmThrowsIfInvalidTaskSpecified() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskD, TaskZ");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(m_alg->execute(), std::runtime_error & e, std::string(e.what()),
                            "Invalid task specified in TaskExecutionOrder: TaskZ");
  }

  void testTaskBasedAlgorithmThrowsIfDuplicateTaskSpecified() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskD, TaskA");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(m_alg->execute(), std::runtime_error & e, std::string(e.what()),
                            "Duplicate task specified in TaskExecutionOrder, this is not yet supported: TaskA");
  }

  void testTaskBasedAlgorithmABCDAlternateOutput() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskC, TaskDAlt");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {0.0, 1.667, 3.333, 5.0, 6.667});
  }

  // Simulate being called through algorithm dialog
  void testTaskBasedAlgorithmWorksWithConsecutiveCalls() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskC, TaskD");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    m_alg->setProperty("TaskExecutionOrder", "TaskA, TaskB, TaskD");
    m_alg->execute();
    Mantid::API::MatrixWorkspace_sptr outputWS = m_alg->getProperty("OutputWorkspace");
    compareVectors(outputWS->readY(0), {2.50, 7.50, 12.50, 17.50, 22.50});
  }

  void testTaskBasedAlgorithmDoesNotMutateInput() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");
    m_alg->execute();
    compareVectors(inputWS->readY(0), {1.0, 2.0, 3.0, 4.0, 5.0});
  }

  void testTaskBasedAlgorithmDoesMutateInputIfSet() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->setProperty("OutputWorkspace", "test_ws");

    auto alg = dynamic_pointer_cast<ToyTaskBasedAlg>(m_alg);
    alg->enableMutableInput();

    m_alg->execute();
    compareVectors(inputWS->readY(0), {5.0, 10.0, 15.0, 20.0, 25.0});
  }

  void testTaskOutputDebugWorkspace() {
    m_alg->initialize();
    m_alg->setAlwaysStoreInADS(false);
    m_alg->setProperty("TaskExecutionOrder", "TaskA");
    Mantid::API::MatrixWorkspace_sptr inputWS = makeMatrixWorkspaceFromVector({1.0, 2.0, 3.0, 4.0, 5.0});
    m_alg->setProperty("InputWorkspace", inputWS);
    m_alg->execute();
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("debug_test_0SUFF"));
  }

private:
  Mantid::API::IAlgorithm_sptr m_alg;
};
