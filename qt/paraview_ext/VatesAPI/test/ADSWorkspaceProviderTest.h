#ifndef ADS_WORKSPACE_PROVIDER_TEST_H_
#define ADS_WORKSPACE_PROVIDER_TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::VATES;
using namespace Mantid::API;

class ADSWorkspaceProviderTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Put A test workspace into the ADS.
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        "WS", IMDWorkspace_sptr(new MockIMDWorkspace()));
  }

  void testCanProvideWorkspace() {
    ADSWorkspaceProvider<IMDWorkspace> wsProvider;
    TSM_ASSERT("Should indicate that the workspace CAN be provided.",
               wsProvider.canProvideWorkspace("WS"));
  }

  void testCannotProvideWorkspaceIfNonExistant() {
    ADSWorkspaceProvider<IMDWorkspace> wsProvider;
    TSM_ASSERT("Should indicate that the workspace CANNOT be provided. It does "
               "not exist.",
               !wsProvider.canProvideWorkspace("WS_X")); // WS_X does not exist!
  }

  void testCannotProvideWorkspaceOfWrongType() {
    // ::setup put a workspace of type IMDWorkspace in the ADS, so even though
    // we are requesting the workspace via the right name, should
    // return false because there is a type mismatch between the stored and
    // requested workspace types.

    ADSWorkspaceProvider<IMDEventWorkspace> wsProvider;
    TSM_ASSERT("Asking for a IMDEventWorkspace. Should indicate that the "
               "workspace CANNOT be provided. see ::setup",
               !wsProvider.canProvideWorkspace("WS"));
  }

  void testFetchWorkspace() {
    ADSWorkspaceProvider<IMDWorkspace> wsProvider;
    TSM_ASSERT_THROWS_NOTHING("Should have fetched WS from ADS",
                              wsProvider.fetchWorkspace("WS"));
    TS_ASSERT(NULL != wsProvider.fetchWorkspace("WS"));
  }

  void testDisposeOfWorkspace() {
    ADSWorkspaceProvider<IMDWorkspace> wsProvider;
    TSM_ASSERT_THROWS_NOTHING("Should have workspace to start with.",
                              wsProvider.fetchWorkspace("WS"));
    TSM_ASSERT_THROWS_NOTHING("Should have removed existing workspace.",
                              wsProvider.disposeWorkspace("WS"));
    TSM_ASSERT_THROWS_ANYTHING("Should not be able to fetch the workspace now",
                               wsProvider.fetchWorkspace("WS"));
  }
};

#endif