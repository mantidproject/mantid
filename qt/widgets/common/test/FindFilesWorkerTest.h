// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_FINDFILESWORKERTEST_H_
#define MANTIDQT_API_FINDFILESWORKERTEST_H_

#include "MantidQtWidgets/Common/FindFilesThreadPoolManagerMockObjects.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QCoreApplication>
#include <QThreadPool>
#include <boost/algorithm/string/predicate.hpp>
#include <cxxtest/TestSuite.h>

using MantidQt::API::FakeMWRunFiles;
using MantidQt::API::FindFilesSearchParameters;
using MantidQt::API::FindFilesSearchResults;
using MantidQt::API::FindFilesWorker;

class FindFilesWorkerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindFilesWorkerTest *createSuite() {
    return new FindFilesWorkerTest();
  }
  static void destroySuite(FindFilesWorkerTest *suite) { delete suite; }

  void test_find_file_with_algorithm() {

    auto parameters = createFileSearch("IRS26173");
    auto worker = new FindFilesWorker(parameters);
    auto widget = createWidget(worker);

    // Act
    executeWorker(worker);

    // Assert
    auto results = widget->getResults();
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 1)
    TS_ASSERT(
        boost::algorithm::contains(results.filenames[0], parameters.searchText))
    TS_ASSERT_EQUALS(results.valueForProperty, results.filenames[0])
  }

  void test_find_run_files() {

    auto parameters = createFileSearch("IRS26173");
    parameters.algorithmName = "";
    parameters.algorithmProperty = "";
    parameters.isForRunFiles = true;
    auto worker = new FindFilesWorker(parameters);
    auto widget = createWidget(worker);

    // Act
    executeWorker(worker);

    // Assert
    auto results = widget->getResults();
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 1)
    TS_ASSERT(
        boost::algorithm::contains(results.filenames[0], parameters.searchText))
    TS_ASSERT_EQUALS(results.valueForProperty, results.filenames[0])
  }

  void test_fail_to_find_file_that_does_not_exist() {

    auto parameters = createFileSearch("ThisFileDoesNotExist");
    auto worker = new FindFilesWorker(parameters);
    auto widget = createWidget(worker);

    // Act
    executeWorker(worker);

    // Assert
    auto results = widget->getResults();
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_DIFFERS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 0)
  }

  void test_fail_to_find_file_when_search_text_is_empty() {

    auto parameters = createFileSearch("");
    auto worker = new FindFilesWorker(parameters);
    auto widget = createWidget(worker);

    // Act
    executeWorker(worker);

    // Assert
    auto results = widget->getResults();
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_DIFFERS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 0)
  }

  void test_no_error_when_search_text_empty_and_optional() {

    auto parameters = createFileSearch("");
    parameters.isOptional = true;
    auto worker = new FindFilesWorker(parameters);
    auto widget = createWidget(worker);

    // Act
    executeWorker(worker);

    // Assert
    auto results = widget->getResults();
    TS_ASSERT(widget->isFinishedSignalRecieved())
    TS_ASSERT_EQUALS(results.error, "")
    TS_ASSERT_EQUALS(results.filenames.size(), 0)
  }

private:
  FindFilesSearchParameters createFileSearch(const std::string &searchText) {
    FindFilesSearchParameters parameters;
    parameters.searchText = searchText;
    parameters.algorithmName = "Load";
    parameters.algorithmProperty = "Filename";
    parameters.isOptional = false;
    parameters.isForRunFiles = false;
    return parameters;
  }

  FakeMWRunFiles *createWidget(FindFilesWorker *worker) {
    auto widget = new FakeMWRunFiles();
    widget->connect(worker, SIGNAL(finished(const FindFilesSearchResults &)),
                    widget,
                    SLOT(inspectThreadResult(const FindFilesSearchResults &)),
                    Qt::QueuedConnection);
    widget->connect(worker, SIGNAL(finished(const FindFilesSearchResults &)),
                    widget, SIGNAL(fileFindingFinished()),
                    Qt::QueuedConnection);
    return widget;
  }

  void executeWorker(FindFilesWorker *worker) {
    auto threadPool = QThreadPool::globalInstance();
    threadPool->start(worker);
    threadPool->waitForDone();
    QCoreApplication::processEvents();
  }
};

#endif /* MANTIDQT_API_FINDFILESWORKERTEST */
