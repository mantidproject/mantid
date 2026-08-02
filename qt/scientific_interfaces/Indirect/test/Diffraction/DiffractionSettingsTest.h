// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Diffraction/DiffractionReduction.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::CustomInterfaces::DiffractionReduction;
using MantidQt::CustomInterfaces::DiffractionSettings;

class DiffractionSettingsTest : public CxxTest::TestSuite {
public:
  void test_read_settings_does_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("CustomInterfaces/DEMON");
    storage.setValue("last_cal_file", "/calibration/file.cal");
    storage.setValue("last_van_files", "1-3,5");
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = DiffractionSettings::readSettings(storage);
    storage.sync();

    TS_ASSERT_EQUALS(values.calibrationFile(), QString("/calibration/file.cal"));
    TS_ASSERT_EQUALS(values.vanadiumFiles(), QString("1-3,5"));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_save_settings_writes_only_documented_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("CustomInterfaces/DEMON");
    storage.setValue("unrelated", "preserved");

    DiffractionSettings::saveSettings(storage, DiffractionSettings("/new/file.cal", "10-12"));
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("last_cal_file").toString(), QString("/new/file.cal"));
    TS_ASSERT_EQUALS(storage.value("last_van_files").toString(), QString("10-12"));
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 3);
    TS_ASSERT(!storage.contains("last_directory"));
  }

private:
  QByteArray fileContents(const QString &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
