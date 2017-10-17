#ifndef MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST_H_
#define MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST_H_

#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManagerMockObjects.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>

using MantidQt::API::FindFilesThreadPoolManager;
using MantidQt::API::FindFilesSearchParameters;
using MantidQt::API::FindFilesSearchResults;
using MantidQt::API::FakeMWRunFiles;

class FindFilesThreadPoolManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindFilesThreadPoolManagerTest *createSuite() { return new FindFilesThreadPoolManagerTest(); }
  static void destroySuite(FindFilesThreadPoolManagerTest *suite) { delete suite; }

  void test_find_single_file() {
    // Arrange
    FakeMWRunFiles widget;
    FindFilesSearchParameters parameters;
    parameters.searchText = "SomeFileThatDontExist";
    parameters.isOptional = false;
    parameters.isForRunFiles = false;
    parameters.algorithmProperty = "Filename";
    parameters.algorithmName = "Load";

    // Act
    FindFilesThreadPoolManager poolManager;
    poolManager.createWorker(&widget, parameters);
    // Block and wait for all the threads to process
    poolManager.waitForDone();

    // Assert
    const auto results = widget.getResults();
    TS_ASSERT(widget.isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
  }

};

#endif /* MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST */
