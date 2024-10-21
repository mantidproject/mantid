// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {

class MockPlotPresenter : public PlotPresenter {
public:
  MockPlotPresenter() : PlotPresenter(nullptr) {};

  MOCK_METHOD(void, clearModel, (), (override));
  MOCK_METHOD(void, setSpectrum, (const Mantid::API::MatrixWorkspace_sptr &, const size_t), (override));
  MOCK_METHOD(void, setScaleLinear, (const AxisID), (override));
  MOCK_METHOD(void, setScaleLog, (const AxisID), (override));
  MOCK_METHOD(void, setPlotErrorBars, (const bool), (override));
  MOCK_METHOD(void, plot, (), (override));
};
} // namespace MantidQt::MantidWidgets
