#ifndef MANTID_API_WORKSPACEWITHINDEXHELPERTEST_H_
#define MANTID_API_WORKSPACEWITHINDEXHELPERTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspacePropertyWithIndexHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class WorkspaceWithIndexHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceWithIndexHelperTest *createSuite() {
    return new WorkspaceWithIndexHelperTest();
  }
  static void destroySuite(WorkspaceWithIndexHelperTest *suite) {
    delete suite;
  }

  void testCreateWithSpectrumNumbers() {
    TS_ASSERT_THROWS_NOTHING(createWithSpectrumNumbers<MatrixWorkspace>());
  }

  void testCreateWithWorkspaceIndices() {
    TS_ASSERT_THROWS_NOTHING(createWithWorkspaceIndices<MatrixWorkspace>());
  }

  void testCreateWithAllIndexTypes() {
    TS_ASSERT_THROWS_NOTHING(createWithAllIndexTypes<MatrixWorkspace>());
  }
};

#endif /* MANTID_API_WORKSPACEWITHINDEXHELPERTEST_H_ */