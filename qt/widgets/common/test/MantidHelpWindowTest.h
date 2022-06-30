// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Instantiator.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include "MantidQtWidgets/Common/MantidHelpWindow.h"

#include <QApplication>
#include <QWidget>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;

class MantidHelpWindowTest : public CxxTest::TestSuite {

public:
  MantidHelpWindowTest() { FrameworkManager::Instance(); }

  static MantidHelpWindowTest *createSuite() { return new MantidHelpWindowTest; }
  static void destroySuite(MantidHelpWindowTest *suite) { delete suite; }

  void setUp() override { m_url = "qthelp://org.mantidproject/doc/interfaces/direct/MSlice.html"; }

  void tearDown() override { assertNoTopLevelWidgets(); }

  void test_the_mantid_help_window_can_be_opened_and_closed_with_a_parent_widget() {}

  void test_the_mantid_help_window_can_be_opened_and_closed_without_a_parent_widget() {
    auto lll = createInterfaceWithParent();
    lll->showPage(m_url);
    assertWidgetCreated();
    lll->shutdown();
  }

  void test_the_mantid_help_window_can_be_opened_and_closed_multiple_times_with_a_parent_widget() {}

  void test_the_mantid_help_window_can_be_opened_and_closed_multiple_times_without_a_parent_widget() {}

private:
  MantidHelpInterface *createInterfaceWithoutParent() {
    const auto factory = new Mantid::Kernel::Instantiator<MantidHelpWindow, MantidHelpInterface>();
    return factory->createUnwrappedInstance();
  }

  MantidHelpInterface *createInterfaceWithParent() {
    const auto factory = new Mantid::Kernel::Instantiator<MantidHelpWindow, MantidHelpInterface, QWidget *>();
    auto parent = new QWidget();
    return factory->createUnwrappedInstance(parent);
  }

  void assertWidgetCreated() { TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size()); }

  void assertNoTopLevelWidgets() { TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size()); }

  std::string m_url;
};
