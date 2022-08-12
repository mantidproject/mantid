// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/MantidHelpWindow.h"

#include <QApplication>
#include <QTimer>
#include <QWidget>

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;

class MantidHelpWindowTest : public CxxTest::TestSuite {

public:
  MantidHelpWindowTest() { FrameworkManager::Instance(); }

  static MantidHelpWindowTest *createSuite() { return new MantidHelpWindowTest; }
  static void destroySuite(MantidHelpWindowTest *suite) { delete suite; }

  void setUp() override { assertNoTopLevelWidgets(); }

  void tearDown() override { assertNoTopLevelWidgets(); }

  void test_the_mantid_help_window_can_be_opened_and_closed_once() { openHelpInterface(); }

  void test_the_mantid_help_window_can_be_opened_and_closed_multiple_times() {
    for (auto i = 0u; i < m_openAttempts; ++i) {
      openHelpInterface();
    }
  }

private:
  void openHelpInterface() {
    // Assert widget is created
    auto helpInterface = new MantidHelpWindow();
    helpInterface->setAttribute(Qt::WA_DeleteOnClose);
    helpInterface->showPage(m_url);
    assertWidgetCreated();

    // Assert widget is closed
    // We don't understand why sendPostedEvents doesn't work, why it needs to be a singleShot,
    // and why processEvents needs to be called twice. Requires more investigation into the
    // Qt code.
    QTimer::singleShot(0, helpInterface, &QWidget::close);
    QApplication::instance()->processEvents();
    QApplication::instance()->processEvents();

    assertNoTopLevelWidgets();
  }

  void assertWidgetCreated() { TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size()); }

  void assertNoTopLevelWidgets() {
    for (auto const &widget : QApplication::topLevelWidgets()) {
      std::cout << widget->metaObject()->className() << std::endl;
    }
    TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size());
  }

  std::size_t m_openAttempts{5u};
  std::string m_url{"qthelp://org.mantidproject/doc/interfaces/direct/MSlice.html"};
};
