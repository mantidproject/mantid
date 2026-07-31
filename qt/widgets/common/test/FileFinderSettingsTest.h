// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::API::FileFinderSettings;
using MantidQt::API::FileFinderWidget;
using MantidQt::MantidWidgets::DataSelector;

class FileFinderSettingsTest : public CxxTest::TestSuite {
public:
  static FileFinderSettingsTest *createSuite() { return new FileFinderSettingsTest(); }
  static void destroySuite(FileFinderSettingsTest *suite) { delete suite; }

  void test_read_and_restore_do_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("FileFinder");
    storage.setValue("last_directory", "/read/only/directory");
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = FileFinderWidget::readSettings(storage);
    FileFinderWidget widget;
    widget.restoreSettings(values);
    storage.sync();

    TS_ASSERT_EQUALS(values.lastDirectory(), QString("/read/only/directory"));
    TS_ASSERT_EQUALS(widget.captureSettings().lastDirectory(), QString("/read/only/directory"));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_save_writes_only_last_directory() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("FileFinder");
    storage.setValue("unrelated", "preserved");

    FileFinderWidget widget;
    widget.saveSettings(storage, FileFinderSettings("/saved/directory"));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("last_directory").toString(), QString("/saved/directory"));
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 2);
  }

  void test_data_selector_restores_and_captures_snapshot_without_storage() {
    DataSelector selector;

    selector.restoreSettings(FileFinderSettings("/selector/directory"));

    TS_ASSERT_EQUALS(selector.captureSettings().lastDirectory(), QString("/selector/directory"));
  }

private:
  QByteArray fileContents(const QString &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
