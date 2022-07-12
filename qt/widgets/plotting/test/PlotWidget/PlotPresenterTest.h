// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Plotting/PlotWidget/QtPlotView.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms, int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBoundaries);
}

} // namespace

/// Unit tests for PlotPresenter
class PlotPresenterTest : public CxxTest::TestSuite {
public:
  static PlotPresenterTest *createSuite() { return new PlotPresenterTest(); }

  static void destroySuite(PlotPresenterTest *suite) { delete suite; }

  PlotPresenterTest() { PyImport_ImportModule("mantid.plots"); }

  void test_constructor() { TS_ASSERT_THROWS_NOTHING(QtPlotView(nullptr)); }

  void test_set_spectrum() {
    auto plot = QtPlotView(nullptr);
    auto ws = createMatrixWorkspace(3);

    TS_ASSERT_THROWS_NOTHING(plot.setSpectrum(ws, 1));
  }

  void test_set_x_scale() {
    auto plot = QtPlotView(nullptr);
    auto ws = createMatrixWorkspace(3);
    plot.setSpectrum(ws, 1);

    TS_ASSERT_THROWS_NOTHING(plot.setXScaleType(QtPlotView::AxisScale::LINEAR));
    TS_ASSERT_THROWS_NOTHING(plot.setXScaleType(QtPlotView::AxisScale::LOG));
  }

  void test_set_x_scale_no_workspace() {
    auto plot = QtPlotView(nullptr);

    TS_ASSERT_THROWS_NOTHING(plot.setXScaleType(QtPlotView::AxisScale::LINEAR));
    TS_ASSERT_THROWS_NOTHING(plot.setXScaleType(QtPlotView::AxisScale::LOG));
  }

  void test_set_y_scale() {
    auto plot = QtPlotView(nullptr);
    auto ws = createMatrixWorkspace(3);
    plot.setSpectrum(ws, 1);

    TS_ASSERT_THROWS_NOTHING(plot.setYScaleType(QtPlotView::AxisScale::LINEAR));
    TS_ASSERT_THROWS_NOTHING(plot.setYScaleType(QtPlotView::AxisScale::LOG));
  }

  void test_set_y_scale_no_workspace() {
    auto plot = QtPlotView(nullptr);

    TS_ASSERT_THROWS_NOTHING(plot.setYScaleType(QtPlotView::AxisScale::LINEAR));
    TS_ASSERT_THROWS_NOTHING(plot.setYScaleType(QtPlotView::AxisScale::LOG));
  }
};
