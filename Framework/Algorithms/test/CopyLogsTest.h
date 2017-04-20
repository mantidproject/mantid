#ifndef MANTID_ALGORITHMS_COPYLOGSTEST_H_
#define MANTID_ALGORITHMS_COPYLOGSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CopyLogs.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CopyLogs;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class CopyLogsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CopyLogsTest *createSuite() { return new CopyLogsTest(); }
  static void destroySuite(CopyLogsTest *suite) { delete suite; }

  void test_Init() {
    CopyLogs alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    MatrixWorkspace_sptr inputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr outputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);

    WorkspaceCreationHelper::storeWS("alpha", outputWs);

    // Merge Strategy
    std::string mode("MergeReplaceExisting");

    runAlg(inputWs, outputWs, mode);

    WorkspaceCreationHelper::removeWS(outputWs->getName());
  }

  void test_mergeReplaceExisting() {
    MatrixWorkspace_sptr inputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr outputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);

    WorkspaceCreationHelper::storeWS("alpha", outputWs);

    // logs for input workspace
    addSampleLog(inputWs, "A", "Hello");
    addSampleLog(inputWs, "B", "World");

    // logs for output workspace
    addSampleLog(outputWs, "B", "Universe");
    addSampleLog(outputWs, "C", 1);

    // Merge Strategy
    std::string mode("MergeReplaceExisting");

    runAlg(inputWs, outputWs, mode);

    // check output
    Run run = outputWs->mutableRun();
    TS_ASSERT_EQUALS(run.getLogData("A")->value(), "Hello");
    TS_ASSERT_EQUALS(run.getLogData("B")->value(), "World");
    TS_ASSERT_EQUALS(run.getLogData("C")->value(), "1");

    WorkspaceCreationHelper::removeWS(outputWs->getName());
  }

  void test_mergeKeepExisting() {
    MatrixWorkspace_sptr inputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr outputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);

    WorkspaceCreationHelper::storeWS("alpha", outputWs);

    // logs for input workspace
    addSampleLog(inputWs, "A", "Hello");
    addSampleLog(inputWs, "B", "World");

    // logs for output workspace
    addSampleLog(outputWs, "B", "Universe");
    addSampleLog(outputWs, "C", 1);

    // Merge Strategy
    std::string mode("MergeKeepExisting");

    runAlg(inputWs, outputWs, mode);

    // check output
    Run run = outputWs->mutableRun();
    TS_ASSERT_EQUALS(run.getLogData("A")->value(), "Hello");
    TS_ASSERT_EQUALS(run.getLogData("B")->value(), "Universe");
    TS_ASSERT_EQUALS(run.getLogData("C")->value(), "1");

    WorkspaceCreationHelper::removeWS(outputWs->getName());
  }

  void test_wipeExisting() {
    MatrixWorkspace_sptr inputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr outputWs =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);

    WorkspaceCreationHelper::storeWS("alpha", outputWs);

    // logs for input workspace
    addSampleLog(inputWs, "A", "Hello");
    addSampleLog(inputWs, "B", "World");

    // logs for output workspace
    addSampleLog(outputWs, "B", "Universe");
    addSampleLog(outputWs, "C", 1);

    // Merge Strategy
    std::string mode("WipeExisting");

    runAlg(inputWs, outputWs, mode);

    // check output
    Run run = outputWs->mutableRun();
    TS_ASSERT_EQUALS(run.getLogData("A")->value(), "Hello");
    TS_ASSERT_EQUALS(run.getLogData("B")->value(), "World");
    TS_ASSERT_THROWS_ANYTHING(run.getLogData("C"));

    WorkspaceCreationHelper::removeWS(outputWs->getName());
  }

  // Run the Copy Logs algorithm
  void runAlg(MatrixWorkspace_sptr in, MatrixWorkspace_sptr out,
              const std::string &mode) {
    CopyLogs alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", in));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MergeStrategy", mode));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", out));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
  }

  // Add a string sample log to the workspace
  void addSampleLog(MatrixWorkspace_sptr ws, const std::string &name,
                    const std::string &value) {
    Run &run = ws->mutableRun();
    run.addLogData(new PropertyWithValue<std::string>(name, value));
  }

  // Add a double sample log to the workspace
  void addSampleLog(MatrixWorkspace_sptr ws, const std::string &name,
                    const double value) {
    Run &run = ws->mutableRun();
    run.addLogData(new PropertyWithValue<double>(name, value));
  }
};

#endif /* MANTID_ALGORITHMS_COPYLOGSTEST_H_ */
