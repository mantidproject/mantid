// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/ProcessingAlgoWidget.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::MantidWidgets::ProcessingAlgoWidgetSettings;

class ProcessingAlgoWidgetSettingsTest : public CxxTest::TestSuite {
public:
  void test_readSettings_does_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("Mantid/ProcessingAlgoWidget");
    storage.setValue("LastFile", "/scripts/process.py");
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = ProcessingAlgoWidgetSettings::readSettings(storage);
    storage.sync();

    TS_ASSERT_EQUALS(values.lastFile(), QString("/scripts/process.py"));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_saveSettings_writes_only_last_file() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.beginGroup("Mantid/ProcessingAlgoWidget");
    storage.setValue("unrelated", "preserved");

    ProcessingAlgoWidgetSettings::saveSettings(storage, ProcessingAlgoWidgetSettings("/scripts/new.py"));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("LastFile").toString(), QString("/scripts/new.py"));
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 2);
  }

private:
  QByteArray fileContents(const QString &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
