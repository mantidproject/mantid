// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::CustomInterfaces::IndicesSuggestionsSettings;
using MantidQt::CustomInterfaces::SettingsHelper::SpectroscopySettings;

class SpectroscopySettingsTest : public CxxTest::TestSuite {
public:
  void test_read_settings_does_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("Indirect Settings");
    storage.setValue("restrict-input-by-name", false);
    storage.setValue("plot-error-bars-external", true);
    storage.setValue("load-history", false);
    storage.setValue("developer-feature-flags", QStringList{"flag"});
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = MantidQt::CustomInterfaces::SettingsHelper::readSettings(storage);
    storage.sync();

    TS_ASSERT(!values.restrictInputByName());
    TS_ASSERT(values.externalPlotErrorBars());
    TS_ASSERT(!values.loadHistory());
    TS_ASSERT_EQUALS(values.developerFeatureFlags(), QStringList{"flag"});
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_save_settings_writes_only_documented_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.beginGroup("Indirect Settings");
    storage.setValue("unrelated", "preserved");

    MantidQt::CustomInterfaces::SettingsHelper::saveSettings(
        storage, SpectroscopySettings(false, true, false, QStringList{"flag"}));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 5);
  }

  void test_indices_suggestions_read_is_unchanged_and_save_preserves_other_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("Indices suggestions");
    storage.setValue("Suggestions", QStringList{"1-3", "5"});
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = IndicesSuggestionsSettings::readSettings(storage);
    storage.sync();
    TS_ASSERT_EQUALS(values.suggestions(), QStringList({"1-3", "5"}));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);

    IndicesSuggestionsSettings::saveSettings(storage, IndicesSuggestionsSettings({"7", "9"}));
    storage.sync();
    TS_ASSERT_EQUALS(storage.value("Suggestions").toStringList(), QStringList({"7", "9"}));
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
