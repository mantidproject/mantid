#ifndef MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_
#define MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtAPI/BatchAlgorithmRunner.h"

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

class BatchAlgorithmRunnerTest : public CxxTest::TestSuite
{
  public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static BatchAlgorithmRunnerTest *createSuite() { return new BatchAlgorithmRunnerTest; }
    static void destroySuite(BatchAlgorithmRunnerTest *suite) { delete suite; }

    BatchAlgorithmRunnerTest()
    {
      // To make sure API is initialized properly
      FrameworkManager::Instance();
    }

    void setUp()
    {
      // Create some algorithms
      // Each algorithm depends on the output workspace of the previous
      createWsAlg = AlgorithmManager::Instance().create("CreateSampleWorkspace", -1);
      createWsAlg->initialize();
      createWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Create");
      createWsAlg->setProperty("Function", "Exp Decay");
      createWsAlg->setProperty("XMax", 20.0);
      createWsAlg->setProperty("BinWidth", 1.0);

      cropRuntimeProps["InputWorkspace"] = "BatchAlgorithmRunnerTest_Create";
      cropWsAlg = AlgorithmManager::Instance().create("CropWorkspace", -1);
      cropWsAlg->initialize();
      cropWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Crop");
      cropWsAlg->setProperty("StartWorkspaceIndex", 4);
      cropWsAlg->setProperty("EndWorkspaceIndex", 5);

      scaleRuntimeProps["InputWorkspace"] = "BatchAlgorithmRunnerTest_Crop";
      scaleWsAlg = AlgorithmManager::Instance().create("Scale", -1);
      scaleWsAlg->initialize();
      scaleWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Scale");
      scaleWsAlg->setProperty("Factor", 5.0);
      scaleWsAlg->setProperty("Operation", "Add");
    }

    void test_basicBatch()
    {
      BatchAlgorithmRunner runner(NULL);

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, cropRuntimeProps);
      runner.addAlgorithm(scaleWsAlg, scaleRuntimeProps);

      // Run queue
      TS_ASSERT(runner.executeBatch());

      // Get workspace history
      std::string wsName = "BatchAlgorithmRunnerTest_Scale";
      auto history = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();

      // Check the algorithm history of the workspace matches what should have been done to it
      TS_ASSERT_EQUALS("CreateSampleWorkspace", history.getAlgorithmHistory(0)->name())
      TS_ASSERT_EQUALS("CropWorkspace", history.getAlgorithmHistory(1)->name())
      TS_ASSERT_EQUALS("Scale", history.getAlgorithmHistory(2)->name())
    }

    void test_basicBatchWorkspaceFailure()
    {
      BatchAlgorithmRunner runner(NULL);

      cropRuntimeProps["InputWorkspace"] = "BatchAlgorithmRunner_NoWorkspace";

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, cropRuntimeProps);

      // Run queue
      TS_ASSERT(!runner.executeBatch());
    }

    void test_basicBatchPropertyFailure()
    {
      BatchAlgorithmRunner runner(NULL);

      cropRuntimeProps["NotAValidProperty"] = "sample_data.nxs";

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, cropRuntimeProps);

      // Run queue
      TS_ASSERT(!runner.executeBatch());
    }

  private:
    IAlgorithm_sptr createWsAlg;
    IAlgorithm_sptr cropWsAlg;
    IAlgorithm_sptr scaleWsAlg;

    BatchAlgorithmRunner::AlgorithmRuntimeProps cropRuntimeProps;
    BatchAlgorithmRunner::AlgorithmRuntimeProps scaleRuntimeProps;

};

#endif /* MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_ */
