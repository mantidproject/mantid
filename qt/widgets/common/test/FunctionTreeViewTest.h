// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FunctionTreeView.h"

#include <cxxtest/TestSuite.h>

#include <memory>

using namespace MantidQt::MantidWidgets;

/*
 * This test was created in response to finding an unreliable Read Access
 * Violation when creating the FunctionTreeView. This failure would happen once
 * every 100-200 attempts to instantiate this class.
 *
 * Its cause was a dangling pointer to a manager object being left behind when
 * destructing a FunctionTreeView. This dangling point was still existing in a
 * global static variable (m_managerToFactoryToViews or
 * m_viewToManagerToFactory) in qtpropertybrowser.cpp. When creating a new
 * instance of FunctionTreeView, the memory location would sometimes be reused,
 * causing problems.

 * The solution used to fix this was to do:
 * m_browser->unsetFactoryForManager(m_manager)
 * in the destructor of FunctionTreeView.
 */

class FunctionTreeViewTest : public CxxTest::TestSuite {

public:
  static FunctionTreeViewTest *createSuite() { return new FunctionTreeViewTest; }
  static void destroySuite(FunctionTreeViewTest *suite) { delete suite; }

  void setUp() override { m_numberOfTries = 100u; }

  void tearDown() override { m_functionTreeView.reset(); }

  void test_that_the_FunctionTreeView_can_be_instantiated_many_times_without_instability() {
    for (auto i = 0u; i < m_numberOfTries; ++i)
      m_functionTreeView = std::make_unique<FunctionTreeView>(nullptr, true);
  }

private:
  std::size_t m_numberOfTries;
  std::unique_ptr<FunctionTreeView> m_functionTreeView;
};
