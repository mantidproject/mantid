#ifndef MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_
#define MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_

//TODO: Temp
#include <unistd.h>

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtAPI/BatchAlgorithmRunner.h"

using namespace Mantid::API;

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

    void test_basicBatch()
    {
      MantidQt::API::BatchAlgorithmRunner runner(NULL);

      std::vector<std::string> workspaces = {"ws1", "ws2", "ws3"};
      runner.preRegisterWorkspaces(workspaces);

      IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateSampleWorkspace", -1);
      createWsAlg->initialize();
      createWsAlg->setProperty("OutputWorkspace", "ws1");
      createWsAlg->setProperty("Function", "Exp Decay");

      IAlgorithm_sptr cropWsAlg = AlgorithmManager::Instance().create("CropWorkspace", -1);
      cropWsAlg->initialize();
      cropWsAlg->setProperty("InputWorkspace", "ws1");
      cropWsAlg->setProperty("OutputWorkspace", "ws2");
      cropWsAlg->setProperty("StartWorkspaceIndex", 4);
      cropWsAlg->setProperty("EndWorkspaceIndex", 9);

      IAlgorithm_sptr scaleWsAlg = AlgorithmManager::Instance().create("Scale", -1);
      scaleWsAlg->initialize();
      scaleWsAlg->setProperty("InputWorkspace", "ws2");
      scaleWsAlg->setProperty("OutputWorkspace", "ws3");
      scaleWsAlg->setProperty("Factor", 5.0);
      scaleWsAlg->setProperty("Operation", "Add");

      IAlgorithm_sptr saveWsAlg = AlgorithmManager::Instance().create("SaveNexus", -1);
      saveWsAlg->initialize();
      saveWsAlg->setProperty("InputWorkspace", "ws3");
      saveWsAlg->setProperty("Filename", "BatchProcessedNexus.nxs");

      runner.addAlgorithm(createWsAlg);
      runner.addAlgorithm(cropWsAlg);
      runner.addAlgorithm(scaleWsAlg);
      runner.addAlgorithm(saveWsAlg);

      runner.startBatch();

      //TODO: Temp
      usleep(30000000);
      /* while(runner.isExecuting()) {} */

      //TODO: Wait for and test result
    }
};

#endif /* MANTIDQT_API_BATCHALGORITHMRUNNERTEST_H_ */
