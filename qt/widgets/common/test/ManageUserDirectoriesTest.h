// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include <QApplication>
#include <QSignalSpy>
#include <QtTest>

using Mantid::Kernel::ConfigService;
using MantidQt::API::ManageUserDirectories;

class ManageUserDirectoriesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ManageUserDirectoriesTest *createSuite() { return new ManageUserDirectoriesTest(); }
  static void destroySuite(ManageUserDirectoriesTest *suite) { delete suite; }

  ManageUserDirectoriesTest() {
    // Start ConfigService
    ConfigService::Instance();
  }

  void test_openManageUserDirectories() {
    auto dialog = openNonPersistingManageUserDirectories();

    TS_ASSERT(dialog);
    closeAndWaitForDeletion(dialog);
  }

  void test_openManageUserDirectories_while_open_returns_same_dialog() {
    auto firstTimeDialog = openNonPersistingManageUserDirectories();
    auto secondTimeDialog = openNonPersistingManageUserDirectories();

    TS_ASSERT_EQUALS(firstTimeDialog, secondTimeDialog);
    closeAndWaitForDeletion(firstTimeDialog);
  }

  void test_openManageUserDirectories_reopen_after_closing() {
    // The internal instance pointer needs resetting if the dialog is closed
    // by whatever way this is possible but esc doesn't run closeEvent if its
    // overridden
    auto firstTimeDialog = openNonPersistingManageUserDirectories();
    QSignalSpy deletionSpy(firstTimeDialog, &QObject::destroyed);

    QTest::mouseClick(firstTimeDialog->cancelButton(), Qt::LeftButton);

    TS_ASSERT(deletionSpy.wait());
    // firstTimeDialog is now unsafe
    auto secondTimeDialog = openNonPersistingManageUserDirectories();
    TS_ASSERT(secondTimeDialog);
    closeAndWaitForDeletion(secondTimeDialog);
  }

  void test_openManageUserDirectories_reopen_after_closing_with_esc() {
    // The internal instance pointer needs resetting if the dialog is closed
    // by whatever way this is possible but esc doesn't run closeEvent if its
    // overridden
    auto firstTimeDialog = openNonPersistingManageUserDirectories();
    QSignalSpy deletionSpy(firstTimeDialog, &QObject::destroyed);

    QTest::keyPress(firstTimeDialog, Qt::Key_Escape);
    QTest::keyRelease(firstTimeDialog, Qt::Key_Escape);

    TS_ASSERT(deletionSpy.wait());
    // firstTimeDialog is now unsafe
    auto secondTimeDialog = openNonPersistingManageUserDirectories();
    TS_ASSERT(secondTimeDialog);
    closeAndWaitForDeletion(secondTimeDialog);
  }

private:
  ManageUserDirectories *openNonPersistingManageUserDirectories() {
    auto dialog = ManageUserDirectories::openManageUserDirectories();
    dialog->enableSaveToFile(false);
    return dialog;
  }

  // The dialog is created with Qt::WA_DeleteOnClose, so close() only posts a
  // deferred-delete event. Spin the event loop until the widget is actually
  // destroyed; otherwise the deletion is left pending and is flushed at process
  // exit, after the GUI thread-local state has been torn down, which crashes
  // under Qt6 (~QSurface calls QOpenGLContext::currentContext()).
  void closeAndWaitForDeletion(ManageUserDirectories *dialog) {
    QSignalSpy deletionSpy(dialog, &QObject::destroyed);
    dialog->close();
    TS_ASSERT(deletionSpy.wait());
  }
};
