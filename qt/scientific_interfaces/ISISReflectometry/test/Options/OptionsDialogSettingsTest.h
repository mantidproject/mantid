// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Options/OptionsDialogModel.h"
#include "MantidQtWidgets/Common/QSettingsHelper.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::CustomInterfaces::ISISReflectometry::OptionsDialogSettings;
using namespace MantidQt::MantidWidgets::QSettingsHelper;

class OptionsDialogSettingsTest : public CxxTest::TestSuite {
public:
  void test_read_settings_does_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    setSetting(storage, "ISISReflectometryUI", "WarnProcessAll", true);
    setSetting(storage, "ISISReflectometryUI", "RoundPrecision", 4);
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = OptionsDialogSettings::readSettings(storage);

    TS_ASSERT_EQUALS(values.boolOptions().at("WarnProcessAll"), true);
    TS_ASSERT_EQUALS(values.intOptions().at("RoundPrecision"), 4);
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_save_settings_preserves_unrelated_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("unrelated", "preserved");

    OptionsDialogSettings::saveSettings(storage,
                                        OptionsDialogSettings({{"WarnProcessAll", false}}, {{"RoundPrecision", 6}}));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    auto const values = OptionsDialogSettings::readSettings(storage);
    TS_ASSERT_EQUALS(values.boolOptions().at("WarnProcessAll"), false);
    TS_ASSERT_EQUALS(values.intOptions().at("RoundPrecision"), 6);
  }

private:
  QByteArray fileContents(QString const &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
