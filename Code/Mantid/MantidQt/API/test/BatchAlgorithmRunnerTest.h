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

    /**
     * Configures some algorithms and their runtime properties to be used in tests.
     */
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
      inputFromCreateProps["InputWorkspace"] = "BatchAlgorithmRunnerTest_Create";

      cropWsAlg = AlgorithmManager::Instance().create("CropWorkspace", -1);
      cropWsAlg->initialize();
      cropWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Crop");
      cropWsAlg->setProperty("StartWorkspaceIndex", 4);
      cropWsAlg->setProperty("EndWorkspaceIndex", 5);
      inputFromCropProps["InputWorkspace"] = "BatchAlgorithmRunnerTest_Crop";

      scaleWsAlg = AlgorithmManager::Instance().create("Scale", -1);
      scaleWsAlg->initialize();
      scaleWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Scale");
      scaleWsAlg->setProperty("Factor", 5.0);
      scaleWsAlg->setProperty("Operation", "Add");
      inputFromScaleProps["InputWorkspace"] = "BatchAlgorithmRunnerTest_Scale";
    }

    /**
     * Tests a standard run of algorithms.
     */
    void test_basicBatch()
    {
      BatchAlgorithmRunner runner(NULL);

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, inputFromCreateProps);
      runner.addAlgorithm(scaleWsAlg, inputFromCropProps);

      // Run queue
      TS_ASSERT_EQUALS(runner.queueLength(), 3);
      TS_ASSERT(runner.executeBatch());
      TS_ASSERT_EQUALS(runner.queueLength(), 0);

      // Get workspace history
      std::string wsName = "BatchAlgorithmRunnerTest_Scale";
      auto history = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();

      // Check the algorithm history of the workspace matches what should have been done to it
      TS_ASSERT_EQUALS("CreateSampleWorkspace", history.getAlgorithmHistory(0)->name())
      TS_ASSERT_EQUALS("CropWorkspace", history.getAlgorithmHistory(1)->name())
      TS_ASSERT_EQUALS("Scale", history.getAlgorithmHistory(2)->name())
    }

    /**
     * Tests runs of multiple batches on the same runner.
     */
    void test_basicMultipleBatch()
    {
      BatchAlgorithmRunner runner(NULL);
      std::string wsName = "BatchAlgorithmRunnerTest_Crop";

      // Run 1
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, inputFromCreateProps);
      TS_ASSERT(runner.executeBatch());

      auto historyRun1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();
      TS_ASSERT_EQUALS("CreateSampleWorkspace", historyRun1.getAlgorithmHistory(0)->name())
      TS_ASSERT_EQUALS("CropWorkspace", historyRun1.getAlgorithmHistory(1)->name())

      // Run 2
      runner.addAlgorithm(scaleWsAlg, inputFromCreateProps);
      runner.addAlgorithm(cropWsAlg, inputFromScaleProps);
      TS_ASSERT(runner.executeBatch());

      auto historyRun2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();
      TS_ASSERT_EQUALS("CreateSampleWorkspace", historyRun1.getAlgorithmHistory(0)->name())
      TS_ASSERT_EQUALS("Scale", historyRun2.getAlgorithmHistory(1)->name())
      TS_ASSERT_EQUALS("CropWorkspace", historyRun2.getAlgorithmHistory(2)->name())
    }

    /**
     * Tests failure caused by setting a property such that it fails validation.
     */
    void test_basicBatchWorkspaceFailure()
    {
      BatchAlgorithmRunner runner(NULL);

      inputFromCreateProps["InputWorkspace"] = "BatchAlgorithmRunner_NoWorkspace";

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, inputFromCreateProps);

      // Run queue
      TS_ASSERT(!runner.executeBatch());
      TS_ASSERT_EQUALS(runner.queueLength(), 0);
    }

    /**
     * Tests failure casused by setting a property that does not exist.
     */
    void test_basicBatchPropertyFailure()
    {
      BatchAlgorithmRunner runner(NULL);

      inputFromCreateProps["NotAValidProperty"] = "sample_data.nxs";

      // Add them to the queue
      // Define the input (and inout, if used) WS properties here
      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg, inputFromCreateProps);

      // Run queue
      TS_ASSERT(!runner.executeBatch());
      TS_ASSERT_EQUALS(runner.queueLength(), 0);
    }

  private:
    IAlgorithm_sptr createWsAlg;
    IAlgorithm_sptr cropWsAlg;
    IAlgorithm_sptr scaleWsAlg;

    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromCreateProps;
    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromCropProps;
    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromScaleProps;

};

#endif /* MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_ */
