// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Instantiator.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include "MantidQtWidgets/Common/MantidHelpWindow.h"

#include <QApplication>
#include <QObject>
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

  // void test_the_mantid_help_window_can_be_opened_and_closed_once_without_a_parent_widget() {
  //  openHelpInterfaceWithoutParent();
  //}

  // void test_the_mantid_help_window_can_be_opened_and_closed_once_with_a_parent_widget() {
  //  openHelpInterfaceWithParent();
  //}

  void test_the_mantid_help_window_can_be_opened_and_closed_multiple_times_without_a_parent_widget() {
    for (auto i = 0u; i < m_openAttempts; ++i) {
      openHelpInterfaceWithoutParent();
    }
  }

  // void test_the_mantid_help_window_can_be_opened_and_closed_multiple_times_with_a_parent_widget() {
  //  for (auto i = 0u; i < m_openAttempts; ++i) {
  //    openHelpInterfaceWithParent();
  //  }
  //}

private:
  void openHelpInterfaceWithoutParent() {
    const auto factory = new Mantid::Kernel::Instantiator<MantidHelpWindow, MantidHelpInterface>();

    // Assert widget is created
    auto helpInterface = factory->createUnwrappedInstance();
    helpInterface->setAttribute(Qt::WA_DeleteOnClose);
    helpInterface->showPage(m_url);
    assertWidgetCreated();

    // Assert widget is closed
    MantidQt::API::InterfaceManager::closeHelpWindow();
    QTimer::singleShot(0, helpInterface, &QWidget::close);
    QApplication::sendPostedEvents();
    QApplication::sendPostedEvents();

    assertNoTopLevelWidgets();
  }

  void openHelpInterfaceWithParent() {
    const auto factory = new Mantid::Kernel::Instantiator<MantidHelpWindow, MantidHelpInterface, QWidget *>();

    QWidget *parent = new QWidget();
    parent->setAttribute(Qt::WA_DeleteOnClose);

    // Assert widget is created
    auto helpInterface = factory->createUnwrappedInstance(parent);
    helpInterface->showPage(m_url);
    assertWidgetCreated();

    // Assert widget is closed
    QTimer::singleShot(0, parent, &QWidget::close);
    QApplication::sendPostedEvents();
    QApplication::sendPostedEvents();

    assertNoTopLevelWidgets();
  }

  void assertWidgetCreated() { TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size()); }

  void assertNoTopLevelWidgets() { TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size()); }

  std::size_t m_openAttempts{25u};
  std::string m_url{"qthelp://org.mantidproject/doc/interfaces/direct/MSlice.html"};
};
