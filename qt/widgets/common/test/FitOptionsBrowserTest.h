// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FitOptionsBrowser.h"

#include <cxxtest/TestSuite.h>

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include <memory>

using namespace MantidQt::MantidWidgets;

/*
 * This test was created in response to finding an unreliable Read Access
 * Violation when creating the FitOptionsBrowser. This failure would happen once
 * every 100-200 attempts to instantiate this class.
 *
 * Its cause was a dangling pointer to a manager object being left behind when
 * destructing a FitOptionsBrowser. This dangling point was still existing in a
 * global static variable (m_managerToFactoryToViews or
 * m_viewToManagerToFactory) in qtpropertybrowser.cpp. When creating a new
 * instance of FitOptionsBrowser, the memory location would sometimes be reused,
 * causing problems.

 * The solution used to fix this was to do:
 * m_browser->unsetFactoryForManager(m_manager)
 * in the destructor of FitOptionsBrowser.
 *
 * A further issue caused by uninitialized memory was also fixed, and is covered
 * by this test.
 */

class FitOptionsBrowserTest : public CxxTest::TestSuite {

public:
  static FitOptionsBrowserTest *createSuite() { return new FitOptionsBrowserTest; }
  static void destroySuite(FitOptionsBrowserTest *suite) { delete suite; }

  void setUp() override { m_numberOfTries = 100u; }

  void tearDown() override { m_fitOptionsBrowser.reset(); }

  void test_that_the_FitOptionsBrowser_can_be_instantiated_many_times_without_instability() {
    for (auto i = 0u; i < m_numberOfTries; ++i)
      m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr);
  }

  void test_that_setting_the_fitting_mode_to_sequential_will_then_return_the_sequential_fitting_mode() {
    m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr);

    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SEQUENTIAL);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getCurrentFittingType(), FittingMode::SEQUENTIAL);
  }

  void test_that_setting_the_fitting_mode_to_simultaneous_will_then_return_the_simultaneous_fitting_mode() {
    m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr);

    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SIMULTANEOUS);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getCurrentFittingType(), FittingMode::SIMULTANEOUS);
  }

  void test_read_and_restore_do_not_modify_persistent_storage() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.beginGroup("FitOptions");
    storage.setValue("MaxIterations", "123");
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);
    m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr);

    auto const values = m_fitOptionsBrowser->readSettings(storage);
    m_fitOptionsBrowser->restoreSettings(values);
    storage.sync();

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("MaxIterations"), QString("123"));
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->captureSettings().values().value("MaxIterations"), QString("123"));
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);
  }

  void test_saveSettings_writes_only_snapshot_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    QSettings storage(directory.filePath("settings.ini"), QSettings::IniFormat);
    storage.beginGroup("FitOptions");
    storage.setValue("unrelated", "preserved");
    FitOptionsBrowserSettings values(
        QMap<QString, QString>{{"MaxIterations", "42"}, {"CostFunction", "Least squares"}});

    FitOptionsBrowserSettings::saveSettings(storage, values);
    storage.sync();

    TS_ASSERT_EQUALS(storage.value("MaxIterations").toString(), QString("42"));
    TS_ASSERT_EQUALS(storage.value("CostFunction").toString(), QString("Least squares"));
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 3);
  }

private:
  QByteArray fileContents(const QString &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }

  std::size_t m_numberOfTries;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
};
