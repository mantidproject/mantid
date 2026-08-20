// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/QSettingsChangeAware.h"

#include <QFile>
#include <QLockFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using MantidQt::MantidWidgets::QSettingsChangeAware;

class QSettingsChangeAwareTest : public CxxTest::TestSuite {
public:
  void test_default_constructor_owns_a_qsettings_instance() {
    QSettingsChangeAware writer;

    TS_ASSERT(!writer.changed());
  }

  void test_setValue_skips_an_equal_value_after_ini_type_conversion() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    {
      QSettings seed(filename, QSettings::IniFormat);
      seed.setValue("answer", 42);
      seed.sync();
    }
    auto const contentsBefore = fileContents(filename);
    QSettings storage(filename, QSettings::IniFormat);
    QSettingsChangeAware writer(storage);

    TS_ASSERT(!writer.setValue("answer", 42));

    TS_ASSERT(!writer.changed());
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_setValue_writes_an_absent_or_changed_value() {
    QTemporaryDir directory;
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.setValue("existing", "old");
    storage.sync();
    QSettingsChangeAware writer(storage);

    TS_ASSERT(writer.setValue("missing", "new"));
    TS_ASSERT(writer.setValue("existing", "new"));

    TS_ASSERT(writer.changed());
    TS_ASSERT_EQUALS(storage.value("missing").toString(), QString("new"));
    TS_ASSERT_EQUALS(storage.value("existing").toString(), QString("new"));
  }

  void test_remove_skips_a_missing_key() {
    QTemporaryDir directory;
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("preserved", true);
    storage.sync();
    auto const contentsBefore = fileContents(filename);
    QSettingsChangeAware writer(storage);

    TS_ASSERT(!writer.remove("missing"));

    TS_ASSERT(!writer.changed());
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_remove_removes_an_exact_key() {
    QTemporaryDir directory;
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.setValue("remove", "value");
    storage.setValue("preserved", true);
    QSettingsChangeAware writer(storage);

    TS_ASSERT(writer.remove("remove"));

    TS_ASSERT(!storage.contains("remove"));
    TS_ASSERT(storage.contains("preserved"));
  }

  void test_remove_detects_and_removes_a_group_with_only_descendant_keys() {
    QTemporaryDir directory;
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.setValue("group/child", "value");
    storage.setValue("preserved", true);
    QSettingsChangeAware writer(storage);

    TS_ASSERT(writer.remove("group"));

    TS_ASSERT(writer.changed());
    TS_ASSERT(!storage.contains("group/child"));
    TS_ASSERT(storage.contains("preserved"));
  }

  void test_remove_with_empty_key_removes_the_current_group_only() {
    QTemporaryDir directory;
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.setValue("group/child", "value");
    storage.setValue("preserved", true);
    storage.beginGroup("group");
    QSettingsChangeAware writer(storage);

    TS_ASSERT(writer.remove(""));

    storage.endGroup();
    TS_ASSERT(!storage.contains("group/child"));
    TS_ASSERT(storage.contains("preserved"));
  }

  void test_unchanged_operations_complete_while_the_qsettings_lock_is_held() {
    QTemporaryDir directory;
    auto const filename = directory.filePath("settings.ini");
    {
      QSettings seed(filename, QSettings::IniFormat);
      seed.setValue("answer", 42);
      seed.sync();
    }
    QLockFile lock(filename + ".lock");
    TS_ASSERT(lock.tryLock());

    {
      QSettings storage(filename, QSettings::IniFormat);
      QSettingsChangeAware writer(storage);
      TS_ASSERT(!writer.setValue("answer", 42));
      TS_ASSERT(!writer.remove("missing"));
      TS_ASSERT(!writer.changed());
    }

    TS_ASSERT(lock.isLocked());
  }

private:
  QByteArray fileContents(QString const &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
