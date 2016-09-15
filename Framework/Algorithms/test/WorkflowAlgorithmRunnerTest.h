#ifndef MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_
#define MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/WorkflowAlgorithmRunner.h"

#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

using Mantid::Algorithms::DeleteWorkspace;
using Mantid::Algorithms::WorkflowAlgorithmRunner;

using namespace Mantid::API;

const static double DEFAULT_TEST_VALUE = 2;

class WorkflowAlgorithmRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkflowAlgorithmRunnerTest *createSuite() { return new WorkflowAlgorithmRunnerTest(); }
  static void destroySuite(WorkflowAlgorithmRunnerTest *suite) { delete suite; }

  WorkflowAlgorithmRunnerTest() {
    auto ioMap = WorkspaceFactory::Instance().createTable();
    ioMap->addColumn("str", "InputWorkspace");
    ioMap->setRowCount(1);
    ioMap->cell<std::string>(0, 0) = "OutputWorkspace";
    m_ioMapForScale = ioMap;
  }

  ~WorkflowAlgorithmRunnerTest() {
    deleteWorkspace(m_ioMapForScale);
  }

  void test_UnsetPropertiesThrows() {
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT_THROWS_ANYTHING(algorithm.execute())
    TS_ASSERT(!algorithm.isExecuted())
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

  void test_SimpleRun() {
    auto setupTable = createSetupTableForScale();
    setupTable->setRowCount(1);
    setupTable->getRef<std::string>("Id", 0) = "id1";
    setupTable->getRef<std::string>("InputWorkspace", 0) = "\"input\"";
    setupTable->getRef<std::string>("OutputWorkspace", 0) = "\"output\"";
    const double factor = 0.66;
    setupTable->getRef<double>("Factor", 0) =  factor;
    setupTable->getRef<std::string>("Operation", 0) = "Multiply";
    auto inputWs = createTestWorkspace();
    AnalysisDataService::Instance().add("input", inputWs);
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Algorithm", "Scale"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("SetupTable", setupTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("InputOutputMap", m_ioMapForScale))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT(AnalysisDataService::Instance().doesExist("output"))
    auto outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output");
    TS_ASSERT_EQUALS(outputWs->y(0)[0], DEFAULT_TEST_VALUE * factor)
    deleteWorkspace(inputWs);
    deleteWorkspace(outputWs);
    deleteWorkspace(setupTable);
  }

  void test_Version() {
    WorkflowAlgorithmRunner algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 1)
  }

private:
  ITableWorkspace_sptr m_ioMapForScale;
  static ITableWorkspace_sptr createSetupTableForScale() {
    auto table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "Id");
    table->addColumn("str", "InputWorkspace");
    table->addColumn("str", "OutputWorkspace");
    table->addColumn("double", "Factor");
    table->addColumn("str", "Operation");
    return table;
  }

  template<typename T_sptr>
  static void deleteWorkspace(T_sptr ws) {
    DeleteWorkspace deleter;
    deleter.setChild(true);
    deleter.initialize();
    deleter.setProperty("Workspace", ws);
    deleter.execute();
  }

  static MatrixWorkspace_sptr createTestWorkspace() {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    auto &es = ws->mutableE(0);
    auto &xs = ws->mutableX(0);
    auto &ys = ws->mutableY(0);
    es[0] = std::sqrt(DEFAULT_TEST_VALUE);
    xs[0] = 0;
    ys[0] = DEFAULT_TEST_VALUE;
    return ws;
  }
};


#endif /* MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_ */
