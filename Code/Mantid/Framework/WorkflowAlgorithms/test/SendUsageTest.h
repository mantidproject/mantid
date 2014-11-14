#ifndef MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_
#define MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/SendUsage.h"

using Mantid::WorkflowAlgorithms::SendUsage;
using namespace Mantid::API;

class SendUsageTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SendUsageTest *createSuite() { return new SendUsageTest(); }
  static void destroySuite(SendUsageTest *suite) { delete suite; }

  void test_Init() {
    SendUsage alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("SendUsageTest_OutputWS");

    SendUsage alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_Something() { TSM_ASSERT("You forgot to write a test!", 0); }
};

#endif /* MANTID_WORKFLOWALGORITHMS_SENDUSAGETEST_H_ */