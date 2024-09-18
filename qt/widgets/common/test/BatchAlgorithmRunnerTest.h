// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MockConfiguredAlgorithm.h"

#include <QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::API;
using Mantid::API::AlgorithmRuntimeProps;
using MantidQt::API::BatchAlgorithmRunner;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

class BatchAlgorithmRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchAlgorithmRunnerTest *createSuite() { return new BatchAlgorithmRunnerTest; }
  static void destroySuite(BatchAlgorithmRunnerTest *suite) { delete suite; }

  BatchAlgorithmRunnerTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  /**
   * Configures some algorithms and their runtime properties to be used in
   * tests.
   */
  void setUp() override {
    // Create some algorithms
    // Each algorithm depends on the output workspace of the previous
    createWsAlg = AlgorithmManager::Instance().create("CreateSampleWorkspace", -1);
    createWsAlg->initialize();
    createWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Create");
    createWsAlg->setProperty("Function", "Exp Decay");
    createWsAlg->setProperty("XMax", 20.0);
    createWsAlg->setProperty("BinWidth", 1.0);
    inputFromCreateProps.setPropertyValue("InputWorkspace", "BatchAlgorithmRunnerTest_Create");

    cropWsAlg = AlgorithmManager::Instance().create("CropWorkspace", -1);
    cropWsAlg->initialize();
    cropWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Crop");
    cropWsAlg->setProperty("StartWorkspaceIndex", 4);
    cropWsAlg->setProperty("EndWorkspaceIndex", 5);
    inputFromCropProps.setPropertyValue("InputWorkspace", "BatchAlgorithmRunnerTest_Crop");

    scaleWsAlg = AlgorithmManager::Instance().create("Scale", -1);
    scaleWsAlg->initialize();
    scaleWsAlg->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Scale");
    scaleWsAlg->setProperty("Factor", 5.0);
    scaleWsAlg->setProperty("Operation", "Add");
    inputFromScaleProps.setPropertyValue("InputWorkspace", "BatchAlgorithmRunnerTest_Scale");
  }

  /**
   * Tests a standard run of algorithms.
   */
  void test_basicBatch() {
    BatchAlgorithmRunner runner(nullptr);

    // Add them to the queue
    // Define the input (and inout, if used) WS properties here
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCropProps));

    // Run queue
    TS_ASSERT_EQUALS(runner.queueLength(), 3);
    TS_ASSERT(runner.executeBatch());
    TS_ASSERT_EQUALS(runner.queueLength(), 0);

    // Get workspace history
    std::string wsName = "BatchAlgorithmRunnerTest_Scale";
    auto history = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();

    // Check the algorithm history of the workspace matches what should have
    // been done to it
    TS_ASSERT_EQUALS("CreateSampleWorkspace", history.getAlgorithmHistory(0)->name())
    TS_ASSERT_EQUALS("CropWorkspace", history.getAlgorithmHistory(1)->name())
    TS_ASSERT_EQUALS("Scale", history.getAlgorithmHistory(2)->name())
  }

  /**
   * Tests runs of multiple batches on the same runner.
   */
  void test_basicMultipleBatch() {
    BatchAlgorithmRunner runner(nullptr);
    std::string wsName = "BatchAlgorithmRunnerTest_Crop";

    // Run 1
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    TS_ASSERT(runner.executeBatch());

    auto historyRun1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();
    TS_ASSERT_EQUALS("CreateSampleWorkspace", historyRun1.getAlgorithmHistory(0)->name())
    TS_ASSERT_EQUALS("CropWorkspace", historyRun1.getAlgorithmHistory(1)->name())

    // Run 2
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromScaleProps));
    TS_ASSERT(runner.executeBatch());

    auto historyRun2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();
    TS_ASSERT_EQUALS("CreateSampleWorkspace", historyRun1.getAlgorithmHistory(0)->name())
    TS_ASSERT_EQUALS("Scale", historyRun2.getAlgorithmHistory(1)->name())
    TS_ASSERT_EQUALS("CropWorkspace", historyRun2.getAlgorithmHistory(2)->name())
  }

  /**
   * Tests passing properties via AlgorithmRuntimeProps.
   */
  void test_AlgorithmRuntimeProps() {
    BatchAlgorithmRunner runner(nullptr);

    // Create an algorithm with a separate AlgorithmRuntimeProps for the properties
    auto alg = AlgorithmManager::Instance().create("CreateSampleWorkspace", -1);
    auto props = std::make_unique<AlgorithmRuntimeProps>();
    props->setProperty("OutputWorkspace", "BatchAlgorithmRunnerTest_Create");
    props->setProperty("Function", "Exp Decay");
    props->setProperty("XMax", 20.0);
    props->setProperty("BinWidth", 1.0);
    runner.addAlgorithm(alg, std::move(props));

    // Run queue
    TS_ASSERT(runner.executeBatch());
    TS_ASSERT_EQUALS(runner.queueLength(), 0);

    // Get workspace history
    std::string wsName = "BatchAlgorithmRunnerTest_Create";
    auto history = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)->getHistory();

    // Check the algorithm history of the workspace matches what should have
    // been done to it
    TS_ASSERT_EQUALS("CreateSampleWorkspace", history.getAlgorithmHistory(0)->name())
  }

  /**
   * Tests failure caused by setting a property such that it fails validation.
   */
  void test_basicBatchWorkspaceFailure() {
    BatchAlgorithmRunner runner(nullptr);

    auto props = inputFromCreateProps;
    props.setPropertyValue("InputWorkspace", "BatchAlgorithmRunner_NoWorkspace");

    // Add them to the queue
    // Define the input (and inout, if used) WS properties here
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(std::move(props)));

    // Run queue
    TS_ASSERT(!runner.executeBatch());
    TS_ASSERT_EQUALS(runner.queueLength(), 0);
  }

  /**
   * Tests failure casused by setting a property that does not exist.
   */
  void test_basicBatchPropertyFailure() {
    BatchAlgorithmRunner runner(nullptr);

    auto props = inputFromCreateProps;
    props.setPropertyValue("NotAValidProperty", "sample_data.nxs");

    // Add them to the queue
    // Define the input (and inout, if used) WS properties here
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(std::move(props)));

    // Run queue
    TS_ASSERT(!runner.executeBatch());
    TS_ASSERT_EQUALS(runner.queueLength(), 0);
  }

  /**
   * Tests setting the entire queue in one call
   */
  void test_setQueue() {
    BatchAlgorithmRunner runner(nullptr);

    auto queue = makeQueueWithThreeMockAlgs();
    runner.setQueue(queue);

    TS_ASSERT_EQUALS(runner.queueLength(), 3);

    TS_ASSERT(runner.executeBatch());
    TS_ASSERT_EQUALS(runner.queueLength(), 0);
  }

  /**
   * Tests clearing a queue
   */
  void test_clearQueue() {
    BatchAlgorithmRunner runner(nullptr);

    auto queue = makeQueueWithThreeMockAlgs();
    runner.setQueue(queue);
    runner.clearQueue();

    TS_ASSERT_EQUALS(runner.queueLength(), 0);
  }

  /**
   * The following tests check that notifications are handled
   */

  void test_completedWithThreeAlgs() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    executeThreeAlgs(runner);

    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 0);
    TS_ASSERT_EQUALS(algStartSpy.count(), 3);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 3);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 0);
    // Check the batch error flag is false
    auto args = batchCompleteSpy.takeFirst();
    TS_ASSERT_EQUALS(args.at(0).toBool(), false);
  }

  void test_batchFailedDueToMissingWorkspace() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    executeAlgWithMissingWorkspace(runner);

    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 0);
    TS_ASSERT_EQUALS(algStartSpy.count(), 2);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 1);
    // Check the batch error flag is true
    auto args = batchCompleteSpy.takeFirst();
    TS_ASSERT_EQUALS(args.at(0).toBool(), true);
  }

  void test_batchFailedDueToInvalidProperty() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    executeAlgWithInvalidProperty(runner);

    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 0);
    TS_ASSERT_EQUALS(algStartSpy.count(), 1);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 1);
    // Check the batch error flag is true
    auto args = batchCompleteSpy.takeFirst();
    TS_ASSERT_EQUALS(args.at(0).toBool(), true);
  }

  void test_stopOnFailure() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    executeThreeAlgsWithSecondFailing(runner);

    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 0);
    TS_ASSERT_EQUALS(algStartSpy.count(), 2);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 1);
    // Check the batch error flag is true
    auto args = batchCompleteSpy.takeFirst();
    TS_ASSERT_EQUALS(args.at(0).toBool(), true);
  }

  void test_continuesIfStopOnFailureIsDisabled() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    runner.stopOnFailure(false);
    executeThreeAlgsWithSecondFailing(runner);

    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 0);
    TS_ASSERT_EQUALS(algStartSpy.count(), 3);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 2);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 1);
    // The error flag is false if not stopping on failure
    auto args = batchCompleteSpy.takeFirst();
    TS_ASSERT_EQUALS(args.at(0).toBool(), false);
  }

  void test_cancelBatch_before_executing_queue() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCropProps));

    runner.cancelBatch();
    runner.executeBatch();

    // All algorithms are run if cancelBatch() is called before execution has started
    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 1);
    TS_ASSERT_EQUALS(algStartSpy.count(), 3);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 3);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 0);
  }

  void test_cancelBatchWithEmptyQueueThenAddAlgsToQueue() {
    BatchAlgorithmRunner runner(nullptr);

    QSignalSpy batchCompleteSpy(&runner, &BatchAlgorithmRunner::batchComplete);
    QSignalSpy batchCancelledSpy(&runner, &BatchAlgorithmRunner::batchCancelled);
    QSignalSpy algStartSpy(&runner, &BatchAlgorithmRunner::algorithmStarted);
    QSignalSpy algCompleteSpy(&runner, &BatchAlgorithmRunner::algorithmComplete);
    QSignalSpy algErrorSpy(&runner, &BatchAlgorithmRunner::algorithmError);

    runner.cancelBatch();

    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCropProps));
    runner.executeBatch();

    // The empty queue was cancelled immediately so any subsequent queue is executed as normal
    TS_ASSERT_EQUALS(batchCompleteSpy.count(), 1);
    TS_ASSERT_EQUALS(batchCancelledSpy.count(), 1);
    TS_ASSERT_EQUALS(algStartSpy.count(), 3);
    TS_ASSERT_EQUALS(algCompleteSpy.count(), 3);
    TS_ASSERT_EQUALS(algErrorSpy.count(), 0);
  }

private:
  IAlgorithm_sptr createWsAlg;
  IAlgorithm_sptr cropWsAlg;
  IAlgorithm_sptr scaleWsAlg;

  AlgorithmRuntimeProps inputFromCreateProps;
  AlgorithmRuntimeProps inputFromCropProps;
  AlgorithmRuntimeProps inputFromScaleProps;

  void executeThreeAlgs(BatchAlgorithmRunner &runner) {
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCreateProps));
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCropProps));
    runner.executeBatch();
  }

  void executeThreeAlgsWithSecondFailing(BatchAlgorithmRunner &runner) {
    auto props = inputFromCreateProps;
    props.setPropertyValue("InputWorkspace", "BatchAlgorithmRunner_NoWorkspace");
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(std::move(props)));
    runner.addAlgorithm(scaleWsAlg, std::make_unique<AlgorithmRuntimeProps>(inputFromCropProps));
    runner.executeBatch();
  }

  void executeAlgWithMissingWorkspace(BatchAlgorithmRunner &runner) {
    auto props = inputFromCreateProps;
    props.setPropertyValue("InputWorkspace", "BatchAlgorithmRunner_NoWorkspace");
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(std::move(props)));
    runner.executeBatch();
  }

  void executeAlgWithInvalidProperty(BatchAlgorithmRunner &runner) {
    auto props = inputFromCreateProps;
    props.setPropertyValue("NotAValidProperty", "sample_data.nxs");
    runner.addAlgorithm(createWsAlg);
    runner.addAlgorithm(cropWsAlg, std::make_unique<AlgorithmRuntimeProps>(std::move(props)));
    runner.executeBatch();
  }

  std::unique_ptr<AlgorithmRuntimeProps> emptyProperties() { return std::make_unique<AlgorithmRuntimeProps>(); }

  std::deque<IConfiguredAlgorithm_sptr> makeQueueWithThreeMockAlgs() {
    auto mockAlg = std::make_shared<MockConfiguredAlgorithm>(emptyProperties());
    ON_CALL(*mockAlg, algorithm).WillByDefault(Return(createWsAlg));
    return std::deque<IConfiguredAlgorithm_sptr>{mockAlg, mockAlg, mockAlg};
  }
};
