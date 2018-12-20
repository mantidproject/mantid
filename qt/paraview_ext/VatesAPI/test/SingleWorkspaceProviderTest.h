#ifndef VATES_API_SINGLE_WORKSPACE_PROVIDER_TEST_H_
#define VATES_API_SINGLE_WORKSPACE_PROVIDER_TEST_H_

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/SingleWorkspaceProvider.h"
#include <boost/pointer_cast.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;

class SingleWorkspaceProviderTest : public CxxTest::TestSuite {
public:
  void test_that_correctly_initiated_provider_can_provide() {
    // Arrange
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    Mantid::VATES::SingleWorkspaceProvider provider(workspace);
    // Act
    auto canProvide = provider.canProvideWorkspace("");
    // Assert
    TSM_ASSERT("Should be able to provide a workspace", canProvide);
  }

  void test_that_incorrectly_initiated_provider_cannot_provide() {
    // Arrange
    Mantid::API::IMDWorkspace_sptr emptyWorkspace;
    Mantid::VATES::SingleWorkspaceProvider provider(emptyWorkspace);
    // Act
    auto canProvide = provider.canProvideWorkspace("");
    // Assert
    TSM_ASSERT("Should not be able to provide a workspace", !canProvide);
  }

  void test_that_workspace_can_be_fetched() {
    // Arrange
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    Mantid::VATES::SingleWorkspaceProvider provider(workspace);
    // Act
    auto fetchedWorkspace = provider.fetchWorkspace("");
    // Assert
    TSM_ASSERT("Should have a handle on a MDHistoWorkspace",
               boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(
                   fetchedWorkspace));
  }
};

#endif
