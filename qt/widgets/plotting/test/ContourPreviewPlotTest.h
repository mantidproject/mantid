// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Plotting/ContourPreviewPlot.h"

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

  ContourPreviewPlotTest() { PyImport_ImportModule("mantid.plots"); }

  void setUp() override { m_contourPlot = std::make_unique<ContourPreviewPlot>(); }

  void tearDown() override { m_contourPlot.reset(); }

  void test_that_a_ContourPreviewPlot_is_instantiated_without_an_active_workspace() { TS_ASSERT(m_contourPlot); }

  void test_getAxisRange_without_workspace() {
    const auto [ymin, ymax] = m_contourPlot->getAxisRange(AxisID::YLeft);
    TS_ASSERT_DELTA(0.0, ymin, 1e-06);
    TS_ASSERT_DELTA(1.0, ymax, 1e-06);
  }

  void test_that_setWorkspace_will_set_the_active_workspace_for_the_contour_plot() {
    auto const workspace = createMatrixWorkspace(3);
    TS_ASSERT_THROWS_NOTHING(m_contourPlot->setWorkspace(workspace));

    const auto [ymin, ymax] = m_contourPlot->getAxisRange(AxisID::YLeft);
    TS_ASSERT_DELTA(0.5, ymin, 1e-06);
    TS_ASSERT_DELTA(3.5, ymax, 1e-06);
  }

private:
  std::unique_ptr<ContourPreviewPlot> m_contourPlot;
};
