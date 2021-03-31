// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/Qwt/ContourPreviewPlot.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms, int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBoundaries);
}

} // namespace

/// Unit tests for ContourPreviewPlot
class ContourPreviewPlotTest : public CxxTest::TestSuite {
public:
  static ContourPreviewPlotTest *createSuite() { return new ContourPreviewPlotTest(); }

  static void destroySuite(ContourPreviewPlotTest *suite) { delete suite; }

  void setUp() override { m_contourPlot = std::make_unique<ContourPreviewPlot>(); }

  void tearDown() override { m_contourPlot.reset(); }

  void test_that_a_ContourPreviewPlot_is_instantiated_without_an_active_workspace() {
    TS_ASSERT(!m_contourPlot->getActiveWorkspace());
  }

  void test_that_getPlot2D_will_get_the_contour_plot() { TS_ASSERT(m_contourPlot->getPlot2D()); }

  void test_that_setWorkspace_will_set_the_active_workspace_for_the_contour_plot() {
    auto const workspace = createMatrixWorkspace(3);

    m_contourPlot->setWorkspace(workspace);

    TS_ASSERT(m_contourPlot->getActiveWorkspace());
    TS_ASSERT_EQUALS(m_contourPlot->getActiveWorkspace(), workspace);
  }

  void test_that_setPlotVisible_will_hide_the_plot_when_it_is_passed_false() {
    m_contourPlot->setPlotVisible(false);
    TS_ASSERT(!m_contourPlot->isPlotVisible());
  }

private:
  std::unique_ptr<ContourPreviewPlot> m_contourPlot;
};
