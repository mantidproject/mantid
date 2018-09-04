#ifndef MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_
#define MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/WorkflowAlgorithmRunner.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/DeleteWorkspace.h"

using Mantid::Algorithms::DeleteWorkspace;
using Mantid::Algorithms::WorkflowAlgorithmRunner;

using namespace Mantid::API;

const static double DEFAULT_TEST_VALUE = 2;

class WorkflowAlgorithmRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkflowAlgorithmRunnerTest *createSuite() {
    return new WorkflowAlgorithmRunnerTest();
  }
  static void destroySuite(WorkflowAlgorithmRunnerTest *suite) { delete suite; }

  WorkflowAlgorithmRunnerTest() {
    auto ioMap = WorkspaceFactory::Instance().createTable();
    ioMap->addColumn("str", "InputWorkspace");
    ioMap->setRowCount(1);
    ioMap->cell<std::string>(0, 0) = "OutputWorkspace";
    m_ioMapForScale = ioMap;
  }

  ~WorkflowAlgorithmRunnerTest() { deleteWorkspace(m_ioMapForScale); }

  void test_CircularDependenciesThrows() {
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(2);
    setupTable->getRef<std::string>("Id", 0) = "flow1";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "out2";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "out1";
    const double scaling1 = 0.03;
    setupTable->getRef<double>("Factor", 0) = scaling1;
    setupTable->getRef<std::string>("Id", 1) = "flow2";
    setupTable->getRef<std::string>("InputWorkspace", 1) = "out1";
    setupTable->getRef<std::string>("OutputWorkspace", 1) = "out2";
    const double scaling2 = 0.09;
    setupTable->getRef<double>("Factor", 1) = scaling2;
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_ANYTHING(algorithm.execute())
    TS_ASSERT(!algorithm.isExecuted())
  }

  void test_ComplexRun() {
    // Data flow: input3->id3->id2->id1->output1; input3->id3->id5->output5;
    // input4->id4->output4
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(5);
    setupTable->getRef<std::string>("Id", 0) = "id1";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "id2";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "\"output1\"";
    const double scaling1 = 2.79;
    setupTable->getRef<double>("Factor", 0) = scaling1;
    setupTable->getRef<std::string>("Id", 1) = "id2";
    setupTable->getRef<std::string>("InputWorkspace", 1) = "id3";
    setupTable->getRef<std::string>("OutputWorkspace", 1) = "output2";
    const double scaling2 = -72.5;
    setupTable->getRef<double>("Factor", 1) = scaling2;
    setupTable->getRef<std::string>("Id", 2) = "id3";
    setupTable->getRef<std::string>("InputWorkspace", 2) = "\"input3\"";
    setupTable->getRef<std::string>("OutputWorkspace", 2) = "output3";
    const double scaling3 = 0.23;
    setupTable->getRef<double>("Factor", 2) = scaling3;
    setupTable->getRef<std::string>("Id", 3) = "id4";
    setupTable->getRef<std::string>("InputWorkspace", 3) = "\"input4\"";
    setupTable->getRef<std::string>("OutputWorkspace", 3) = "\"output4\"";
    const double scaling4 = 4.01;
    setupTable->getRef<double>("Factor", 3) = scaling4;
    setupTable->getRef<std::string>("Id", 4) = "id5";
    setupTable->getRef<std::string>("InputWorkspace", 4) = "id3";
    setupTable->getRef<std::string>("OutputWorkspace", 4) = "\"output5\"";
    const double scaling5 = -5.54;
    setupTable->getRef<double>("Factor", 4) = scaling5;
    auto inputWs3 = createTestWorkspace("input3");
    auto inputWs4 = createTestWorkspace("input4");
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    assertOutputWorkspace("output1", scaling3 * scaling2 * scaling1);
    assertOutputWorkspace("output2", scaling3 * scaling2);
    assertOutputWorkspace("output3", scaling3);
    assertOutputWorkspace("output4", scaling4);
    assertOutputWorkspace("output5", scaling3 * scaling5);
    deleteWorkspace(inputWs3);
    deleteWorkspace(inputWs4);
  }

  void test_ForcedOutputAsInput() {
    // Data flow: input->spider2->output2; input->spider2->mantid1->output1
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(2);
    setupTable->getRef<std::string>("Id", 0) = "mantid1";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "spider2";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "\"output1\"";
    const double scaling1 = 42;
    setupTable->getRef<double>("Factor", 0) = scaling1;
    setupTable->getRef<std::string>("Id", 1) = "spider2";
    setupTable->getRef<std::string>("InputWorkspace", 1) = "\"input\"";
    setupTable->getRef<std::string>("OutputWorkspace", 1) = "\"output2\"";
    const double scaling2 = 2.3;
    setupTable->getRef<double>("Factor", 1) = scaling2;
    auto inputWs = createTestWorkspace("input");
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    assertOutputWorkspace("output1", scaling2 * scaling1);
    assertOutputWorkspace("output2", scaling2);
    deleteWorkspace(inputWs);
  }

  void test_Init() {
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
  }

  void test_Name() {
    WorkflowAlgorithmRunner algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "WorkflowAlgorithmRunner")
  }

  void test_NonExistentInputThrows() {
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(1);
    setupTable->getRef<std::string>("Id", 0) = "failingJob";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "notInSetupTable";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "\"output1\"";
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_ANYTHING(algorithm.execute())
    TS_ASSERT(!algorithm.isExecuted())
  }

  void test_SimpleRun() {
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(1);
    setupTable->getRef<std::string>("Id", 0) = "id1";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "\"input\"";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "\"output\"";
    const double factor = 0.66;
    setupTable->getRef<double>("Factor", 0) = factor;
    auto inputWs = createTestWorkspace("input");
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    assertOutputWorkspace("output", factor);
    deleteWorkspace(inputWs);
    deleteWorkspace(setupTable);
  }

  void test_UnsetPropertiesThrows() {
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT_THROWS_ANYTHING(algorithm.execute())
    TS_ASSERT(!algorithm.isExecuted())
  }

  void test_Version() {
    WorkflowAlgorithmRunner algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 1)
  }

private:
  ITableWorkspace_sptr m_ioMapForScale;

  static void assertOutputWorkspace(const std::string &name,
                                    const double factor) {
    TS_ASSERT(AnalysisDataService::Instance().doesExist(name))
    auto outputWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    TS_ASSERT_EQUALS(outputWs->y(0)[0], DEFAULT_TEST_VALUE * factor)
    deleteWorkspace(outputWs);
  }

  static ITableWorkspace_sptr createSetupTableForScale() {
    auto table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "Id");
    table->addColumn("str", "InputWorkspace");
    table->addColumn("str", "OutputWorkspace");
    table->addColumn("double", "Factor");
    // The "Operation" property will be left to its default value and thus
    // omitted here.
    return table;
  }

  static MatrixWorkspace_sptr createTestWorkspace(const std::string &name) {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    auto &es = ws->mutableE(0);
    auto &xs = ws->mutableX(0);
    auto &ys = ws->mutableY(0);
    es[0] = std::sqrt(DEFAULT_TEST_VALUE);
    xs[0] = 0;
    ys[0] = DEFAULT_TEST_VALUE;
    AnalysisDataService::Instance().add(name, ws);
    return ws;
  }

  template <typename T_sptr> static void deleteWorkspace(T_sptr ws) {
    DeleteWorkspace deleter;
    deleter.setChild(true);
    deleter.initialize();
    deleter.setProperty("Workspace", ws);
    deleter.execute();
  }
};

#endif /* MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_ */
