// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/MessageDisplay.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::API::AbstractAlgorithmInputHistory;
using MantidQt::API::AlgorithmInputHistorySettings;
using MantidQt::MantidWidgets::MessageDisplay;
using MantidQt::MantidWidgets::MessageDisplaySettings;

class ConfigurableSettingsTest : public CxxTest::TestSuite {
public:
  void test_algorithm_history_read_and_restore_do_not_modify_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("TestAlgorithms/Load/Filename", "input.nxs");
    storage.setValue("TestAlgorithms/LastDirectory", "/data");
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);
    TestAlgorithmInputHistory history;

    auto const values = history.readSettings(storage);

    TS_ASSERT_EQUALS(values.lastInput().value("Load").value("Filename"), QString("input.nxs"));
    TS_ASSERT_EQUALS(values.previousDirectory(), QString("/data"));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);

    history.restoreSettings(values);
    TS_ASSERT_EQUALS(history.previousInput("Load", "Filename"), QString("input.nxs"));
    TS_ASSERT_EQUALS(history.getPreviousDirectory(), QString("/data"));
    auto const captured = history.captureSettings();
    TS_ASSERT_EQUALS(captured.lastInput(), values.lastInput());
    TS_ASSERT_EQUALS(captured.previousDirectory(), values.previousDirectory());
  }

  void test_algorithm_history_save_preserves_unrelated_keys_and_replaces_algorithm_values() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.setValue("TestAlgorithms/Load/Stale", "remove");
    storage.setValue("unrelated", "preserved");
    AlgorithmInputHistorySettings::InputHistory inputHistory;
    inputHistory["Load"]["Filename"] = "new.nxs";
    TestAlgorithmInputHistory history;

    history.saveSettings(storage, AlgorithmInputHistorySettings(inputHistory, "/new"));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("TestAlgorithms/Load/Filename").toString(), QString("new.nxs"));
    TS_ASSERT(!storage.contains("TestAlgorithms/Load/Stale"));
    TS_ASSERT_EQUALS(storage.value("TestAlgorithms/LastDirectory").toString(), QString("/new"));
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
  }

  void test_message_display_read_restore_capture_and_save_are_separate() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("MessageDisplayPriority", 0);
    storage.setValue("MessageDisplayLineCountMax", 321);
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);
    MessageDisplay display;

    auto const values = display.readSettings(storage);

    TS_ASSERT_EQUALS(values.logLevel(), 0);
    TS_ASSERT_EQUALS(values.maximumLineCount(), 321);
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);

    display.restoreSettings(values);
    TS_ASSERT_EQUALS(display.captureSettings().maximumLineCount(), 321);

    display.saveSettings(storage, MessageDisplaySettings(0, 654));
    storage.sync();
    TS_ASSERT_EQUALS(storage.value("MessageDisplayLineCountMax").toInt(), 654);
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
  }

private:
  class TestAlgorithmInputHistory : public AbstractAlgorithmInputHistory {
  public:
    TestAlgorithmInputHistory() : AbstractAlgorithmInputHistory("TestAlgorithms") {}
  };

  QByteArray fileContents(QString const &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
