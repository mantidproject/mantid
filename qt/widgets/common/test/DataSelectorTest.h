// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::API::FileFinderWidget;
using MantidQt::API::FindFilesSearchResults;
using MantidQt::MantidWidgets::DataSelector;

class DataSelectorTest : public CxxTest::TestSuite {
public:
  static DataSelectorTest *createSuite() { return new DataSelectorTest(); }
  static void destroySuite(DataSelectorTest *suite) { delete suite; }

  void test_file_that_does_not_exist_sets_file_problem_and_does_not_throw() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const missingFile = directory.filePath("does_not_exist.nxs").toStdString();

    DataSelector selector;
    auto *fileFinder = selector.findChild<FileFinderWidget *>("rfFileInput");
    TS_ASSERT(fileFinder);
    QSignalSpy autoLoadedSpy(&selector, &DataSelector::filesAutoLoaded);

    // Simulate the file finder reporting a file which the Load algorithm will reject
    FindFilesSearchResults results;
    results.filenames = {missingFile};
    results.valueForProperty = missingFile;

    TS_ASSERT_THROWS_NOTHING(QMetaObject::invokeMethod(fileFinder, "inspectThreadResult", Qt::DirectConnection,
                                                       Q_ARG(FindFilesSearchResults, results)));
    TS_ASSERT(!fileFinder->getFileProblem().isEmpty());
    TS_ASSERT_EQUALS(autoLoadedSpy.count(), 0);
  }
};
