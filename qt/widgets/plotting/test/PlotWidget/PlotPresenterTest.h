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
#include "MantidQtWidgets/Plotting/PlotWidget/IPlotView.h"
#include "MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h"
#include "MantidQtWidgets/Plotting/PlotWidget/QtPlotView.h"
#include "MockPlotView.h"

#include <gmock/gmock.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

/// Unit tests for PlotPresenter
class PlotPresenterTest : public CxxTest::TestSuite {
public:
  static PlotPresenterTest *createSuite() { return new PlotPresenterTest(); }

  static void destroySuite(PlotPresenterTest *suite) { delete suite; }

  void test_set_scale_linear_x() {
    auto view = testing::NiceMock<MockPlotView>();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLinear(AxisID::XBottom)).Times(1);

    presenter.setScaleLinear(AxisID::XBottom);
  }

  void test_set_scale_linear_y() {
    auto view = testing::NiceMock<MockPlotView>();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLinear(AxisID::YLeft)).Times(1);

    presenter.setScaleLinear(AxisID::YLeft);
  }

  void test_set_scale_log_x() {
    auto view = testing::NiceMock<MockPlotView>();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLog(AxisID::XBottom)).Times(1);

    presenter.setScaleLog(AxisID::XBottom);
  }

  void test_set_scale_log_y() {
    auto view = testing::NiceMock<MockPlotView>();
    auto presenter = PlotPresenter(&view);

    EXPECT_CALL(view, setScaleLog(AxisID::YLeft)).Times(1);

    presenter.setScaleLog(AxisID::YLeft);
  }
};
