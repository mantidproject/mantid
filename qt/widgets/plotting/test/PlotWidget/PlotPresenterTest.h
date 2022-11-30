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
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h"
#include "MockPlotModel.h"
#include "MockPlotView.h"

#include <gmock/gmock.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using testing::Return;

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms, int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBoundaries);
}

/// Unit tests for PlotPresenter
class PlotPresenterTest : public CxxTest::TestSuite {
public:
  static PlotPresenterTest *createSuite() { return new PlotPresenterTest(); }

  static void destroySuite(PlotPresenterTest *suite) { delete suite; }

  void test_set_spectrum() {
    auto view = MockPlotView();
    auto model = std::make_unique<MockPlotModel>();
    auto ws = createMatrixWorkspace(3);
    auto const wsIndex = 1;

    EXPECT_CALL(*model, setSpectrum(ws, wsIndex)).Times(1);

    auto presenter = PlotPresenter(&view, std::move(model));

    presenter.setSpectrum(ws, wsIndex);
  }

  void test_plot() {
    auto view = MockPlotView();
    auto model = makeModel();
    auto workspaces = std::vector<MatrixWorkspace_sptr>{createMatrixWorkspace(3)};
    auto const wsIndices = std::vector<int>{1};
    auto const plotErrors = true;

    EXPECT_CALL(*model, getWorkspaces()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(*model, getWorkspaceIndices()).Times(1).WillOnce(Return(wsIndices));
    EXPECT_CALL(*model, getPlotErrorBars()).Times(1).WillOnce(Return(plotErrors));
    EXPECT_CALL(view, plot(workspaces, wsIndices, plotErrors)).Times(1);

    auto presenter = PlotPresenter(&view, std::move(model));

    presenter.plot();
  }

  void test_set_scale_linear_x() {
    auto view = MockPlotView();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLinear(AxisID::XBottom)).Times(1);

    presenter.setScaleLinear(AxisID::XBottom);
  }

  void test_set_scale_linear_y() {
    auto view = MockPlotView();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLinear(AxisID::YLeft)).Times(1);

    presenter.setScaleLinear(AxisID::YLeft);
  }

  void test_set_scale_log_x() {
    auto view = MockPlotView();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLog(AxisID::XBottom)).Times(1);

    presenter.setScaleLog(AxisID::XBottom);
  }

  void test_set_scale_log_y() {
    auto view = MockPlotView();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLog(AxisID::YLeft)).Times(1);

    presenter.setScaleLog(AxisID::YLeft);
  }

  void test_set_plot_error_bars() {
    auto view = MockPlotView();
    auto model = makeModel();

    EXPECT_CALL(*model, setPlotErrorBars(true)).Times(1);
    auto presenter = PlotPresenter(&view, std::move(model));

    presenter.setPlotErrorBars(true);
  }

private:
  std::unique_ptr<MockPlotModel> makeModel() { return std::make_unique<MockPlotModel>(); }
};
