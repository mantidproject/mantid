// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/PlotWidget/IPlotView.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {

class MockPlotView : public IPlotView {
public:
  virtual ~MockPlotView() = default;

  MOCK_METHOD(void, setScaleLinear, (const AxisID), (override));
  MOCK_METHOD(void, setScaleLog, (const AxisID), (override));
  MOCK_METHOD(void, plot,
              (const std::vector<Mantid::API::MatrixWorkspace_sptr> &, const std::vector<int> &, const bool),
              (override));
};
} // namespace MantidQt::MantidWidgets
