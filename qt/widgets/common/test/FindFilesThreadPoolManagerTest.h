#ifndef MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST_H_
#define MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST_H_

#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManagerMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using MantidQt::API::FindFilesThreadPoolManager;
using MantidQt::API::FindFilesSearchParameters;
using MantidQt::API::FindFilesSearchResults;
using MantidQt::API::FakeMWRunFiles;
using MantidQt::API::FakeFindFilesThread;

class FindFilesThreadPoolManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindFilesThreadPoolManagerTest *createSuite() {
    return new FindFilesThreadPoolManagerTest();
  }
  static void destroySuite(FindFilesThreadPoolManagerTest *suite) {
    delete suite;
  }

  void test_find_single_file() {
    int argc = 1;
    char name[] = "DummyTestingApplication";
    char *argv = name;
    QCoreApplication app(argc, &argv);
    // Arrange
    FakeMWRunFiles *widget = new FakeMWRunFiles();

    // The parameters of the search
    FindFilesSearchParameters parameters;
    parameters.searchText = "SomeFileName";
    parameters.isOptional = false;
    parameters.isForRunFiles = false;
    parameters.algorithmProperty = "Filename";
    parameters.algorithmName = "Load";

    // The results we should get back
    FindFilesSearchResults exp_results;
    exp_results.filenames.push_back("FoundFile");

    auto fakeAllocator =
        [&exp_results](const FindFilesSearchParameters &parameters) {
          return new FakeFindFilesThread(parameters, exp_results);
        };
    FindFilesThreadPoolManager poolManager;
    poolManager.setAllocator(fakeAllocator);

    // Act
    poolManager.createWorker(widget, parameters);
    // Block and wait for all the threads to process
    poolManager.waitForDone();
    QCoreApplication::processEvents();

    // Assert
    const auto results = widget->getResults();

    TS_ASSERT(!poolManager.isSearchRunning())
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 1)
    TS_ASSERT_EQUALS(results.filenames[0], exp_results.filenames[0])
  }

  void test_starting_new_search_cancels_currently_running_search() {
    int argc = 1;
    char name[] = "DummyTestingApplication";
    char *argv = name;
    QCoreApplication app(argc, &argv);
    // Arrange
    FakeMWRunFiles widget;

    // The parameters of the search
    FindFilesSearchParameters parameters;
    parameters.searchText = "SomeFileName";
    parameters.isOptional = false;
    parameters.isForRunFiles = false;
    parameters.algorithmProperty = "Filename";
    parameters.algorithmName = "Load";

    // The results we should get back
    FindFilesSearchResults exp_results;
    exp_results.filenames.push_back("FoundFile");

    auto fakeAllocatorNoResults =
        [](const FindFilesSearchParameters &parameters) {
          // Run a thread that returns nothing and takes 1000 milliseconds to do
          // so
          return new FakeFindFilesThread(parameters, FindFilesSearchResults(),
                                         1000);
        };

    auto fakeAllocatorSomeResults =
        [&exp_results](const FindFilesSearchParameters &parameters) {
          // Run a thread that returns something and takes 100 milliseconds to
          // do so
          return new FakeFindFilesThread(parameters, exp_results);
        };

    // Act
    FindFilesThreadPoolManager poolManager;

    // Create a long running worker that will return nothing.
    poolManager.setAllocator(fakeAllocatorNoResults);
    poolManager.createWorker(&widget, parameters);

    // Create a new worker which is shorter and will return a result. This
    // cancels the currently running job. It will be left to run, but will
    // be disconnected from the widget.
    poolManager.setAllocator(fakeAllocatorSomeResults);
    poolManager.createWorker(&widget, parameters);

    // Block and wait for all the threads to process
    poolManager.waitForDone();
    QCoreApplication::processEvents();

    // Assert
    const auto results = widget.getResults();

    TS_ASSERT(!poolManager.isSearchRunning())
    TS_ASSERT(widget.isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 1)
    TS_ASSERT_EQUALS(results.filenames[0], exp_results.filenames[0])
  }
};

#endif /* MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTEST */
