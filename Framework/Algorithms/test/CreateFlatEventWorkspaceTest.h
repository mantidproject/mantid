#ifndef MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CreateFlatEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

using Mantid::Algorithms::CreateFlatEventWorkspace;

class CreateFlatEventWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateFlatEventWorkspaceTest *createSuite() {
    return new CreateFlatEventWorkspaceTest();
  }
  static void destroySuite(CreateFlatEventWorkspaceTest *suite) {
    delete suite;
  }

  void test_pass() { TS_ASSERT(true); }

  void xtest_Init() {
    CreateFlatEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void xtest_exec() {
    // Name of the output workspace.
    std::string outWSName("CreateFlatEventWorkspaceTest_OutputWS");

    CreateFlatEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Mantid::API::MatrixWorkspace_sptr ws;

    TS_ASSERT_THROWS_NOTHING(
        ws = Mantid::API::AnalysisDataService::Instance()
                 .retrieveWS<Mantid::API::MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // TODO: Check the results
    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

  void xtest_Something() { TSM_ASSERT("You forgot to write a test!", 0); }
};

#endif /* MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACETEST_H_ */
